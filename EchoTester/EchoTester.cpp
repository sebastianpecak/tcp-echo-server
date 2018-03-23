// EchoTester.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <type_traits>

///////////////////////////////////////////////////////////////////////////////
#pragma comment(lib, "ws2_32.lib")
///////////////////////////////////////////////////////////////////////////////
static WSADATA wsaData = { 0 };

///////////////////////////////////////////////////////////////////////////////
bool InitializeWinSoc() {
    if (WSAStartup(0x0202, &wsaData)) {
        printf("Could not initialize WinSoc.");
        return false;
    }

    if (wsaData.wVersion != 0x0202) { //Wrong Winsock version?
        WSACleanup();
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {

    if (argc < 3) {
        printf("Error\n");
        printf("Usage: %s host:string port:int\n", argv[0]);
        getchar();
        return 0;
    }

    InitializeWinSoc();

    // Create new socket.
    SOCKET mySocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (mySocket != INVALID_SOCKET) {
        addrinfo* addrInfo = NULL;
        INT res = getaddrinfo(argv[1], argv[2], NULL, &addrInfo);
        if (res == 0) {
            res = connect(mySocket, addrInfo->ai_addr, sizeof(std::remove_pointer_t<decltype(addrInfo->ai_addr)>));
            if (res == 0) {

                const char exampleMsg[] = "This is example message. Java sucks.\n";
                send(mySocket, exampleMsg, sizeof(exampleMsg) - 1, 0);
                //Sleep(1000);
                send(mySocket, exampleMsg, sizeof(exampleMsg) - 1, 0);
                //Sleep(1000);
                send(mySocket, exampleMsg, sizeof(exampleMsg) - 1, 0);

                char recvBuf[512];
                int recved = recv(mySocket, recvBuf, sizeof(recvBuf) - 1, 0);
                recvBuf[recved] = 0;

                printf("Got: '%s'", recved);
            }
            else
                printf("Could not connect to remote host.\n");
        }
        else {
            printf("Could not get address info.\n");
        }

        closesocket(mySocket);
    }
    else {
        printf("Could not create new socket.\n");
    }

    WSACleanup();
    return 0;
}

