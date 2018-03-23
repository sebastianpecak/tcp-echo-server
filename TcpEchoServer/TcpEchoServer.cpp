// TcpEchoServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <thread>
#include <chrono>
#include <future>
#include <fstream>
#include <string>
#include <sstream>
#include <ctime>
#include <iostream>

///////////////////////////////////////////////////////////////////////////////
using std::string;
using std::stringstream;

///////////////////////////////////////////////////////////////////////////////
#pragma comment(lib, "ws2_32.lib")

///////////////////////////////////////////////////////////////////////////////
static WSADATA wsaData = { 0 };

///////////////////////////////////////////////////////////////////////////////
static std::string GetThreadID() {
    stringstream ss;
    std::this_thread::get_id()._To_text(ss);

    return ss.str();
}

///////////////////////////////////////////////////////////////////////////////
static std::string GetDateTime() {
    auto x = std::chrono::system_clock::now();
    std::time_t tme = std::chrono::system_clock::to_time_t(x);

    char timeBuf[64];
    strftime(timeBuf, sizeof(timeBuf), "%F %T", gmtime(&tme));
    return timeBuf;
}

///////////////////////////////////////////////////////////////////////////////
class Log {
    static std::mutex _mutex;

protected:
    template <typename... T>
    static inline void PrintFormatted(const char* fromat, const char* type, T&&... args) {
        std::lock_guard<std::mutex> lock(_mutex);
        printf("[%6s | %s | %-8s]> ", GetThreadID().c_str(), GetDateTime().c_str(), type);
        printf(fromat, std::forward<T&&>(args)...);
        puts("");
    }

public:
    template <typename... T>
    static void Info(const char* fromat, T&&... args) {
        PrintFormatted(fromat, "", std::forward<T&&>(args)...);
    }

    template <typename... T>
    static void Warning(const char* text, T&&... args) {
        PrintFormatted(text, "WARNING", std::forward<T&&>(args)...);
    }

    template <typename... T>
    static void Error(const char* text, T&&... args) {
        PrintFormatted(text, "ERROR", std::forward<T&&>(args)...);
    }
};

///////////////////////////////////////////////////////////////////////////////
std::mutex Log::_mutex;

///////////////////////////////////////////////////////////////////////////////
bool InitializeWinSoc() {
	if (WSAStartup(0x0202, &wsaData)) {
		Log::Error("Could not initialize WinSoc.");
		return false;
	}

	if (wsaData.wVersion != 0x0202) { //Wrong Winsock version?
		WSACleanup();
		return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
static SOCKET MainSocket = 0;

///////////////////////////////////////////////////////////////////////////////
bool InitMainSocket(u_short listeningPort) {
	SOCKADDR_IN addr; // The address structure for a TCP socket

	addr.sin_family = AF_INET;      // Address family
	addr.sin_port = htons(listeningPort);   // Assign port to this socket

									 //Accept a connection from any IP using INADDR_ANY
									 //You could pass inet_addr("0.0.0.0") instead to accomplish the 
									 //same thing. If you want only to watch for a connection from a 
									 //specific IP, specify that //instead.
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	MainSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // Create socket

	if (MainSocket == INVALID_SOCKET)
	{
		return false; //Don't continue if we couldn't create a //socket!!
	}

	if (bind(MainSocket, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		//We couldn't bind (this will happen if you try to bind to the same  
		//socket more than once)
		return false;
	}

}

///////////////////////////////////////////////////////////////////////////////
static void DisplayHelp() {
	Log::Info("Version: %.01f", VERSION);
    Log::Info("Tool usage:");
    Log::Info("\ttool.exe portNumber\n");
}

///////////////////////////////////////////////////////////////////////////////
static void NewConnectionHandler(SOCKET socket) {
    Log::Info("Started new connection handler for socket: %u.", socket);

    // Configure socket.
    DWORD timeout = 5 * 1000;
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

    // Receive request.
    do {
        char buffer[512];
        Log::Info("Reading data...");
        int recieved = recv(socket, buffer, sizeof(buffer) - 1, 0);
        if (recieved > 0) {
            buffer[recieved] = 0;
            Log::Info("Sending data back...: '%s'", buffer);
            if (send(socket, buffer, recieved, 0) == -1)
                Log::Error("Data sending error!");
        }
        else {
            Log::Warning("Could not received data, due to: %d", recieved);
            break;
        }
    } while (1);

    Log::Info("Ended connection handler for socket: %u.", socket);
}

///////////////////////////////////////////////////////////////////////////////
// Vector of thread to not detach threads (could be useful for threads managment).
std::vector<std::thread> GlobalThreads;

///////////////////////////////////////////////////////////////////////////////
void HandleNewConnection(SOCKET newSocket) {
    GlobalThreads.emplace_back(std::thread(NewConnectionHandler, newSocket));
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
    if (argc < 2) {
        DisplayHelp();
        return 0;
    }

	InitializeWinSoc();
	InitMainSocket(atoi(argv[1]));

    stringstream x;
    x << "Your PC supports ";
    x << std::thread::hardware_concurrency();
    x << " concurrent threads.";
    Log::Info(x.str().c_str());

	int iResult = listen(MainSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
        Log::Error("listen failed.");
		closesocket(MainSocket);
		WSACleanup();
		return 1;
	}

	Log::Info("Starting main loop...");
	while (true) {

		// Accept a client socket
        Log::Info("Waiting for client's connection...");
        SOCKET incommingConnection = accept(MainSocket, NULL, NULL);
        Log::Info("New connection on socket: %u", incommingConnection);
		HandleNewConnection(incommingConnection);
	}

	WSACleanup();
    return 0;
}

