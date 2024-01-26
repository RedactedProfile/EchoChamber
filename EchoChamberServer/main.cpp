#ifndef UNICODE
#define UNICODE
#endif

#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN

#define DEFAULT_PORT "27012"
#define DEFAULT_BUFLEN 1024
#define MAX_CLIENTS 30

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

WSADATA 
    wsaData;

SOCKET 
    master = INVALID_SOCKET, 
    new_socket = INVALID_SOCKET,
    client_socket[MAX_CLIENTS]{ INVALID_SOCKET }, 
    s = INVALID_SOCKET;

struct addrinfo
    * result = NULL,
    * ptr = NULL, 
    hints;

struct sockaddr_in
    server,
    address;

int 
    iresult,
    activity, 
    addrlen, 
    i,
    valread;

char 
    *buffer;

std::string 
    s_port = DEFAULT_PORT;

// socket descriptors
fd_set 
    readfds;




int main(int argc, char** argv)
{
    std::cout << "Server" << std::endl;

    if (argc == 2) {
        s_port = std::string(argv[1]);
    }

    const char* c_port = s_port.c_str();

    buffer = (char*)malloc((DEFAULT_BUFLEN + 1) * sizeof(char));

    ZeroMemory(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    for (i = 0; i < MAX_CLIENTS; ++i) 
        client_socket[i] = INVALID_SOCKET;

    iresult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iresult != 0) {
        std::cerr << "WSAStartup Failed " << iresult << std::endl;
        exit(EXIT_FAILURE);
    }

    iresult = getaddrinfo("0.0.0.0", c_port, &hints, &result);
    if (iresult != 0) {
        std::cerr << "getaddrinfo failed " << iresult << std::endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    master = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (master == INVALID_SOCKET) {
        std::cerr << "Error at socket(): " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    iresult = bind(master, result->ai_addr, (int)result->ai_addrlen);
    if (iresult == SOCKET_ERROR) {
        std::cerr << "bind failed with error " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        closesocket(master);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);

    std::cout << "Listening on 0.0.0.0:" << c_port << std::endl;
    if (listen(master, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed with error " << WSAGetLastError() << std::endl;
        closesocket(master);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    addrlen = sizeof(struct sockaddr_in);
    
    std::cout << "waiting for connections..." << std::endl;


    char newbuffer[DEFAULT_BUFLEN];

    // infinite loop
    for (;;) {
        // clear descriptor set
        FD_ZERO(&readfds);

        // add master to descriptor set
        FD_SET(master, &readfds);

        // add children to descriptor set
        for (i = 0; i < MAX_CLIENTS; ++i) {
            s = client_socket[i];
            if (s != INVALID_SOCKET) {
                FD_SET(s, &readfds);
            }
        }

        activity = select(0, &readfds, NULL, NULL, NULL);
        if (activity == SOCKET_ERROR) {
            std::cerr << "multiplex select failed with error: " << WSAGetLastError() << std::endl;
            exit(EXIT_FAILURE);
        }

        // handle incoming connections
        if (FD_ISSET(master, &readfds)) {
            if ((new_socket = accept(master, (struct sockaddr*)&address, (int*)&addrlen)) < 0) {
                std::cerr << "Unaccepted Client Connection?" << std::endl;
                exit(EXIT_FAILURE);
            }

            std::cout << "Accepted client connection " << new_socket << " from " << inet_ntoa(address.sin_addr) << std::endl;

            const char* greeting_message = "Welcome, Wayward Stranger. \r\n";
            if (send(new_socket, greeting_message, strlen(greeting_message), 0) != strlen(greeting_message)) {
                std::cerr << "Greeting Message failed to send" << std::endl;
                exit(EXIT_FAILURE);
            }
            std::cout << "Welcome message sent" << std::endl;

            for (i = 0; i < MAX_CLIENTS; ++i) {
                if (client_socket[i] == INVALID_SOCKET) {
                    client_socket[i] = new_socket;
                    std::cout << "Adding socket to list at " << i << std::endl;
                    break;
                }
            }

        }

        // handle input from clients
        for (i = 0; i < MAX_CLIENTS; ++i) {
            if (client_socket[i] == INVALID_SOCKET) 
                continue;

            s = client_socket[i];

            if (FD_ISSET(s, &readfds)) {
                getpeername(s, (struct sockaddr*)&address, (int*)&addrlen);

                // check if disconnecting
                std::cout << "Checking client for data" << std::endl;

                valread = recv(s, newbuffer, DEFAULT_BUFLEN, 0);

                if (valread == SOCKET_ERROR) {
                    int error_code = WSAGetLastError();
                    if (error_code == WSAECONNRESET) {
                        // client disconnected ungracefully
                        std::cout << "Client disconnected unexpectedly: " << inet_ntoa(address.sin_addr) << std::endl;
                        closesocket(s);
                        client_socket[i] = INVALID_SOCKET;
                    }
                    else
                    {
                        std::cerr << "recv failed with error code " << error_code << std::endl;
                        exit(EXIT_FAILURE);
                    }
                }
                else if (valread == 0) {
                    // accept client disconnected
                    std::cout << "Client disconnected: " << inet_ntoa(address.sin_addr) << std::endl;
                    closesocket(s);
                    client_socket[i] = INVALID_SOCKET;
                }
                else {
                    newbuffer[valread] = '\0';
                    std::cout << "from " << inet_ntoa(address.sin_addr) << ": " << newbuffer << std::endl;

                    std::string sendbuf = "Got your message: ";
                    sendbuf.append(newbuffer);

                    send(s, sendbuf.c_str(), sendbuf.length(), 0);
                }
            }
        }
    }

    closesocket(s);
    WSACleanup();

    std::cout << "Done ==" << std::endl;
    exit(EXIT_SUCCESS);
}