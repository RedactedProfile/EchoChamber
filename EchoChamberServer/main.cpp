#ifndef UNICODE
#define UNICODE
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN

#define DEFAULT_PORT "27012"
#define DEFAULT_BUFLEN 512

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

WSADATA wsaData;
SOCKET ListenSocket = INVALID_SOCKET, ClientSocket = INVALID_SOCKET;
int iResult;

struct addrinfo *result = NULL, *ptr = NULL, hints;






int main(int argc, char** argv)
{
    std::cout << "Server" << std::endl;

    std::string s_port = DEFAULT_PORT;
    if (argc == 2) {
        s_port = std::string(argv[1]);
    }

    const char* c_port = s_port.c_str();

    ZeroMemory(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;


    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup Failed " << iResult << std::endl;
        return 0;
    }

    iResult = getaddrinfo("0.0.0.0", c_port, &hints, &result);
    if (iResult != 0) {
        std::cerr << "getaddrinfo failed " << iResult << std::endl;
        WSACleanup();
        return 0;
    }


    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        std::cerr << "Error at socket(): " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        WSACleanup();
        return 0;
    }

    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        std::cerr << "bind failed with error " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 0;
    }

    freeaddrinfo(result);

    std::cout << "Listening on 0.0.0.0:" << c_port << std::endl;
    if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed with error " << WSAGetLastError() << std::endl;
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    
    std::cout << "waiting for connections..." << std::endl;
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        std::cerr << "accept failed " << WSAGetLastError() << std::endl;
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Connection received" << std::endl;

    char recvbuf[DEFAULT_BUFLEN];
    int iSendResult;
    int recvbuflen = DEFAULT_BUFLEN;
    do {

        iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            std::cout << "Bytes received " << iResult << std::endl;

            iSendResult = send(ClientSocket, recvbuf, iResult, 0);
            if (iSendResult == SOCKET_ERROR) {
                std::cerr << "send failed: " << WSAGetLastError() << std::endl;
                closesocket(ClientSocket);
                WSACleanup();
                return 1;
            }

            std::cout << "bytes sent " << iSendResult << std::endl;
        }
        else if (iResult == 0) {
            std::cout << "Connection closing" << std::endl;
        }
        else {
            std::cerr << "recv error: " << WSAGetLastError() << std::endl;
            closesocket(ClientSocket);
            WSACleanup();
            return 1;
        }

    } while (iResult > 0);



    std::cout << "Done ==" << std::endl;
    return 1;
}