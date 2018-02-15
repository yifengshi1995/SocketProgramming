#define _WIN32_WINNT 0x501

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <string>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_PORT "50001"

int __cdecl main(int argc, char* argv[])
{
	WSADATA wsaData;
	int iResult, iSendResult;

	SOCKET connectSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL, *ptr = NULL, hints;

	// Validate the parameters
	if (argc != 2) {
		printf("usage: %s server-name\n", argv[0]);
		return 1;
	}

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSA Startup failed: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
		// Create a SOCKET for connecting to server
		connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

		if (connectSocket == INVALID_SOCKET) {
			printf("Error at socket(): %ld\n", WSAGetLastError());
			freeaddrinfo(result);
			WSACleanup();
			return 1;
		}

		// Connect to server.
		iResult = connect(connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(connectSocket);
			connectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (connectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}

	while (true) {
		printf("Enter the operator (+, -, *, /), followed by two integers (separate with space): \n");
		printf("Exmaple: \"* 2 5\" stands for \"2 * 5\".\n");
		char input[32];
		fgets(input, 32, stdin);
		
		//Calculate the number of spaces to ensure that user has entered in correct format
		int numSpace = 0;
		if (input[0] == ' ') {
			printf("Invalid input. Please do not type spaces at the beginning.\n");
			continue;
		}

		bool cont = false;
		for (int i = 1; input[i] != '\0'; i++) {
			
			if (input[i] == ' ') {
				if (input[i - 1] == ' ') {
					printf("Invalid input. Continuous spaces detected.\n");
					cont = true;
					break;
				}else
					numSpace++;
			}
		}
		if (cont) continue;

		if (numSpace != 2) {
			printf("Invalid input. Please use the format \"operator number1 number2\".\n");
			printf("The input should only have three components with two spaces, no extra spaces at the end.\n");
			continue;
		}

		//Ask the user to check the input again, and decide to move forward or re-enter.
		printf("Do you want to re-enter your expression ? Press Y / y for yes or N / n for no : \n");
		char confirm[4];
		fgets(confirm, 4, stdin);
		confirm[strlen(confirm) - 1] = '\0';

		//If this input is not Y/y or N/n, ask to re-enter until able to be recognized.
		while (strcmp(confirm, "Y") != 0 && strcmp(confirm, "y") != 0 && strcmp(confirm, "N") != 0 && strcmp(confirm, "n") != 0) {
			printf("Not recognized. Press Y / y for yes or N / n for no : \n");
			fgets(confirm, 4, stdin);
			confirm[strlen(confirm) - 1] = '\0';
		}

		//If Y/y, go back to first step
		if (strcmp(confirm, "Y") == 0 || strcmp(confirm, "y") == 0)
			continue;
			
		
		//Otherwise send the expression to server.
		iSendResult = send(connectSocket, input, strlen(input) + 1, 0);
		if (iSendResult == SOCKET_ERROR) {
			printf("send failed: %d\n", WSAGetLastError());
			closesocket(connectSocket);
			WSACleanup();
			return 1;
		}

		//Get the result
		char result[64];
		iResult = recv(connectSocket, result, sizeof(result), 0);
		if (iResult < 0) {
			printf("recv failed: %d\n", WSAGetLastError());
			//shutdown and cleanup
			iResult = shutdown(connectSocket, SD_SEND);
			if (iResult == SOCKET_ERROR) {
				printf("shutdown failed: %d\n", WSAGetLastError());
				closesocket(connectSocket);
				WSACleanup();
				return 1;
			}
			closesocket(connectSocket);
			WSACleanup();
			return 1;
		}
		else 
			printf(result);
		

		//Ask user to do it again or terminate
		printf("Do you want to do it again? Press Y / y for yes or N / n for no : \n");

		fgets(confirm, 4, stdin);
		confirm[strlen(confirm) - 1] = '\0';

		//If this input is not Y/y or N/n, ask to re-enter until able to be recognized.
		while (strcmp(confirm, "Y") != 0 && strcmp(confirm, "y") != 0 && strcmp(confirm, "N") != 0 && strcmp(confirm, "n") != 0) {
			printf("Not recognized. Press Y / y for yes or N / n for no : \n");
			fgets(confirm, 4, stdin);
			confirm[strlen(confirm) - 1] = '\0';
		}

		//If N/n, terminate the connection.
		if (strcmp(confirm, "N") == 0 || strcmp(confirm, "n") == 0) {
			break;
		}
	}
	//shutdown and cleanup
	iResult = shutdown(connectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed: %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return 1;
	}
	closesocket(connectSocket);
	WSACleanup();
	return 0;
}

