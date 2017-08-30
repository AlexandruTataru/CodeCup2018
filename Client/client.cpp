#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#include <iostream>

#define DEFAULT_BUFLEN 512

char recvbuf[DEFAULT_BUFLEN];

using namespace std;

int main(int argc, char **argv)
{
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL,
            *ptr = NULL,
            hints;
    //char *sendbuf = "this is a test";
    int iResult;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo("localhost", argv[1], &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
                               ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    // Send an initial buffer
   /* iResult = send( ConnectSocket, sendbuf, (int)strlen(sendbuf), 0 );
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }*/

    //printf("Bytes Sent: %ld\n", iResult);

    // shutdown the connection since no more data will be sent
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    memset(recvbuf, 0, DEFAULT_BUFLEN);
    recv(ConnectSocket, recvbuf, 2, 0);
    printf("Received: %s\n", recvbuf);

    memset(recvbuf, 0, DEFAULT_BUFLEN);
    recv(ConnectSocket, recvbuf, 2, 0);
    printf("Received: %s\n", recvbuf);

    memset(recvbuf, 0, DEFAULT_BUFLEN);
    recv(ConnectSocket, recvbuf, 2, 0);
    printf("Received: %s\n", recvbuf);

    memset(recvbuf, 0, DEFAULT_BUFLEN);
    recv(ConnectSocket, recvbuf, 2, 0);
    printf("Received: %s\n", recvbuf);

    memset(recvbuf, 0, DEFAULT_BUFLEN);
    recv(ConnectSocket, recvbuf, 2, 0);
    printf("Received: %s\n", recvbuf);

    memset(recvbuf, 0, DEFAULT_BUFLEN);
    recv(ConnectSocket, recvbuf, 5, 0);
    printf("Received: %s\n", recvbuf);

    iResult = send(ConnectSocket, "A5=13", 5, 0 );
    if (iResult == SOCKET_ERROR)
        printf("send failed with error: %d\n", WSAGetLastError());
    else
        printf("Send %d bytes successfully\n", iResult);

    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}
