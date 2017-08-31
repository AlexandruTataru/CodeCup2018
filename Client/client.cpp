#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#include <iostream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <time.h>

using namespace std;

std::string receiveData(const SOCKET& socket, const size_t& len)
{
	char *recvbuf = new char(len + 1);
	memset(recvbuf, 0, len + 1);
	recv(socket, recvbuf, len, 0);
	std::string receivedData = std::string(recvbuf);
	cout << "Received data: " << receivedData.c_str() << endl;
	return receivedData;
}

void sendData(const SOCKET& socket, const std::string& data)
{
	cout << "Send data: " << data.c_str() << endl;
	send(socket, data.c_str(), data.size(), 0);
}

std::vector<size_t> allowedValues = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

std::vector<std::string> allowedMoves = { "A1", "A2", "A3", "A4", "A5", "A6", "A7", "A8",
"B1", "B2", "B3", "B4", "B5", "B6", "B7",
"C1", "C2", "C3", "C4", "C5", "C6",
"D1", "D2", "D3", "D4", "D5",
"E1", "E2" ,"E3", "E4",
"F1", "F2", "F3",
"G1", "G2",
"H1" };

std::string processMove(const std::string& move)
{
	if (move != "Start")
		allowedMoves.erase(std::remove(allowedMoves.begin(), allowedMoves.end(), move.substr(0, 2)), allowedMoves.end());

	size_t value = allowedValues[rand() % allowedValues.size()];
	allowedValues.erase(std::remove(allowedValues.begin(), allowedValues.end(), value), allowedValues.end());
	std::stringstream ss;
	ss << allowedMoves[rand() % allowedMoves.size()];
	ss << "=";
	ss << value;
	allowedMoves.erase(std::remove(allowedMoves.begin(), allowedMoves.end(), ss.str().substr(0, 2)), allowedMoves.end());
	return ss.str();
}

void discardAllowedMove(const std::string& cellID)
{
	allowedMoves.erase(std::remove(allowedMoves.begin(), 
						allowedMoves.end(), 
						cellID), 
						allowedMoves.end());
}

int main(int argc, char **argv)
{
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;

	WSAStartup(MAKEWORD(2, 2), &wsaData);

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	getaddrinfo("localhost", argv[1], &hints, &result);
	srand(time(NULL));

	for (ptr = result; ptr != NULL;ptr = ptr->ai_next) {
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		int iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	std::string blockedCell1 = receiveData(ConnectSocket, 2);
	discardAllowedMove(blockedCell1);
	std::string blockedCell2 = receiveData(ConnectSocket, 2);
	discardAllowedMove(blockedCell2);
	std::string blockedCell3 = receiveData(ConnectSocket, 2);
	discardAllowedMove(blockedCell3);
	std::string blockedCell4 = receiveData(ConnectSocket, 2);
	discardAllowedMove(blockedCell4);
	std::string blockedCell5 = receiveData(ConnectSocket, 2);
	discardAllowedMove(blockedCell5);

	std::string move = receiveData(ConnectSocket, 5);

	while (move != "Quit")
	{
		Sleep(100);
		string response = processMove(move);
		sendData(ConnectSocket, response);
		move = receiveData(ConnectSocket, 5);
	}

	closesocket(ConnectSocket);
	WSACleanup();

	return 0;
}
