#define _WIN32_WINNT 0x501

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <cstring>
#include <string>
#include <stdexcept>

#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT "50001"
#define BUFFER_LENGTH 512

int __cdecl main()
{
	WSADATA wsaData;
	int iResult;

	SOCKET listenSocket = INVALID_SOCKET;
	SOCKET clientSocket = INVALID_SOCKET;

	char recvBuf[BUFFER_LENGTH];
	int iSendResult;
	int recvBufLen = BUFFER_LENGTH;

	struct addrinfo *result = NULL, hints;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSA Startup failed: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	//resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for the server to listen for client connections
	listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (listenSocket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	//Setup the TCP listening socket
	iResult = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
		printf("Listen failed with error: %ld\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	//Accpet a client socket
	clientSocket = accept(listenSocket, NULL, NULL);
	if (clientSocket == INVALID_SOCKET) {
		printf("accept failed: %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	closesocket(listenSocket);

	do {
		printf("Waiting for client...\n");
		iResult = recv(clientSocket, recvBuf, recvBufLen, 0);
		if (iResult < 0) {
			printf("receive failed: %d\n", WSAGetLastError());
			closesocket(clientSocket);
			WSACleanup();
			return 1;
		}
		else if (iResult == 0) {
			iResult = recv(clientSocket, recvBuf, recvBufLen, 0);
			if (iResult < 0) {
				printf("receive failed: %d\n", WSAGetLastError());
				closesocket(clientSocket);
				WSACleanup();
				return 1;
			}
			else if (iResult == 0)
				printf("Connection closing...\n");
			else
				continue;
		}
		else {
			//split the message
			std::string words[3]{"", "", ""};
			int indexWords = 0;
			std::string temp = "";
			char* msg = recvBuf;
			while (*msg != '\0'){
				if (*msg != ' ' && *msg != '\n')
					temp += *msg;
				else {
					words[indexWords] = temp;
					indexWords++;
					temp = "";
				}
				msg++;
			}
			
			//If received 'N', means client want to close the connection.
			//Then close the server. This is the only correct way of termination.
			if (strcmp(words[0].c_str(), "n") == 0 || strcmp(words[0].c_str(), "N") == 0) {
				break;
			}

			char* sendMsg;

			//Condition 1: Invalid operator. Check if not +, -, * /, or not a single character
			char op = words[0][0];
			if ((op != '+' && op != '-' && op != '*' && op != '/') || strlen(words[0].c_str()) != 1) {
				sendMsg = (char*)"ERROR: Invalid Operator\n";
				iSendResult = send(clientSocket, sendMsg, strlen(sendMsg) + 1, 0);
				continue;
			}

			//Condition 2: Operands are not ints
			int left, right;
			try {
				left = std::stoi(words[1]);
				right = std::stoi(words[2]);
			}
			catch (std::invalid_argument &e) {
				sendMsg = (char*)"ERROR: Not A Number\n";
				iSendResult = send(clientSocket, sendMsg, strlen(sendMsg) + 1, 0);
				if (iSendResult == SOCKET_ERROR) {
					printf("send failed: %d\n", WSAGetLastError());
					closesocket(clientSocket);
					WSACleanup();
					return 1;
				}
				continue;
			}
			catch (std::out_of_range &e) {
				sendMsg = (char*)"ERROR: Number Out Of Range\n";
				iSendResult = send(clientSocket, sendMsg, strlen(sendMsg) + 1, 0);
				if (iSendResult == SOCKET_ERROR) {
					printf("send failed: %d\n", WSAGetLastError());
					closesocket(clientSocket);
					WSACleanup();
					return 1;
				}
				continue;
			}

			//Condition 3: Divisor is zero
			if (op == '/' && right == 0) {
				sendMsg = (char*)"ERROR: Divisor Is Zero\n";
				iSendResult = send(clientSocket, sendMsg, strlen(sendMsg) + 1, 0);
				if (iSendResult == SOCKET_ERROR) {
					printf("send failed: %d\n", WSAGetLastError());
					closesocket(clientSocket);
					WSACleanup();
					return 1;
				}
				continue;
			}

			//If no error happens, show the expression and send the result back to client.
			printf("The expression is: %d %c %d\n", left, op, right);
			int result = 0;
			switch (op) {
			case '+':
				result = left + right;
				break;
			case '-':
				result = left - right;
				break;
			case '*':
				result = left * right;
				break;
			case '/':
				result = left / right;
				break;
			}

			std::string sendMsgStr = "The answer is: " + std::to_string(result) + "\n";
			iSendResult = send(clientSocket, sendMsgStr.c_str(), strlen(sendMsgStr.c_str()) + 1, 0);
			if (iSendResult == SOCKET_ERROR) {
				printf("send failed: %d\n", WSAGetLastError());
				closesocket(clientSocket);
				WSACleanup();
				return 1;
			}
		}
	} while (iResult > 0);
	iResult = shutdown(clientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed: %d\n", WSAGetLastError());
		closesocket(clientSocket);
		WSACleanup();
		return 1;
	}

	// cleanup
	closesocket(clientSocket);
	WSACleanup();
	return 0;
}

