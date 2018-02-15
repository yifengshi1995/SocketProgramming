# SocketProgramming

Computer Networking Class Project.
Originally written in Java, it is a C++ interpretion.

### Usage

I used MinGW G++  6.3.0 to compile this program. 

To compile,

```sh
$ g++ TCPServer.cpp -o TCPServer -lws2_32
$ g++ TCPClient.cpp -o TCPClient -lws2_32
```

To run,

Please use two seperated command prompt window.

Run TCPServer first:

```sh
$ TCPServer
```

Then run TCPClient with argument IP address:

```sh
$ TCPClient localhost
```

Then just follow the directions in program to proceed this program.

### Bugs:

Since std::stoi() will automatically prune the non-numerical items, input a number followed by a letter will not be warned.
