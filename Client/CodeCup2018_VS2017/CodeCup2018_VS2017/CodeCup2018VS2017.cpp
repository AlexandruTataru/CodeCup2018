#include "stdafx.h"
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
#include <string>
#include <algorithm>
#include <sstream>
#include <time.h>
#include <map>

using namespace std;

//#define TESTING_SCENARIO

/////////////////////////////////////////////////////////////////////////////
//
// Support structures
//
/////////////////////////////////////////////////////////////////////////////

enum CELL_TYPE
{
	PLAYABLE,
	OWN,
	ENEMY,
	BLOCKED
};

struct Cell
{
	CELL_TYPE cellType;
	int value;
	std::vector<Cell*> neighbors;
	int compoundValue;

	Cell() : cellType(PLAYABLE), value(0), compoundValue(0) {}
};

/////////////////////////////////////////////////////////////////////////////
//
// Base player class with all the required logic
//
/////////////////////////////////////////////////////////////////////////////

class BasePlayer
{
public:
	typedef std::pair<const std::string, const int> Move;
protected:
	std::vector<std::string> allowedMoves;
	std::vector<int> allowedValues;
	std::vector<int> enemyAllowedValues;
	std::map<std::string, Cell*> cellMapping;
	int movesLeft;
public:
	BasePlayer() { resetGame(); }
	~BasePlayer() {
		allowedMoves.clear();
		allowedValues.clear();
		for (auto cell : cellMapping) delete cell.second;
		cellMapping.clear();
	}

	static const Move raw2Move(const std::string& rawMove)
	{
		bool isPlayable = (rawMove.find('=') != string::npos);
		std::string cellID = rawMove.substr(0, 2);
		int cellValue = isPlayable ? std::stoi(rawMove.substr(3)) : 0;
		return Move(cellID, cellValue);
	}

	static const std::string move2Raw(const Move& move)
	{
		std::stringstream ss;
		ss << move.first;
		ss << "=";
		ss << move.second;
		return ss.str();
	}

	virtual void processBlockedMove(const Move& move)
	{
		allowedMoves.erase(std::remove(allowedMoves.begin(),
			allowedMoves.end(),
			move.first),
			allowedMoves.end());
		cellMapping[move.first]->cellType = BLOCKED;
	}

	virtual void processEnemyMove(const Move& move)
	{
		if (move.first == "St") return;
		std::string cellID = move.first;
		int cellValue = move.second;
		cellMapping[cellID]->value = cellValue;
		cellMapping[cellID]->cellType = ENEMY;

		std::vector<std::string> neighbors = getNeighborsIDs(cellID);
		for (auto neighborIDs : neighbors)
		{
			Cell *cell = cellMapping[neighborIDs];
			if(cell->cellType == CELL_TYPE::PLAYABLE) cell->compoundValue -= cellValue;
		}

		allowedMoves.erase(std::remove(allowedMoves.begin(), allowedMoves.end(), cellID), allowedMoves.end());
		enemyAllowedValues.erase(std::remove(enemyAllowedValues.begin(),
			enemyAllowedValues.end(),
			cellValue),
			enemyAllowedValues.end());
	}

	virtual Move nextMove() = 0;

	virtual void resetGame() {
		movesLeft = 15;
		allowedMoves.clear();
		for (auto elem : { "A1", "A2", "A3", "A4", "A5", "A6", "A7", "A8",
			"B1", "B2", "B3", "B4", "B5", "B6", "B7",
			"C1", "C2", "C3", "C4", "C5", "C6",
			"D1", "D2", "D3", "D4", "D5",
			"E1", "E2" ,"E3", "E4",
			"F1", "F2", "F3",
			"G1", "G2",
			"H1" })
			allowedMoves.push_back(elem);
		allowedValues.clear();
		enemyAllowedValues.clear();
		for (auto elem : { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 })
		{
			allowedValues.push_back(elem);
			enemyAllowedValues.push_back(elem);
		}

		for (auto cell : cellMapping) delete cell.second;
		cellMapping.clear();

		for (auto key : allowedMoves)
		{
			Cell *cell = new Cell();
			cellMapping[key] = cell;
		}

		for (auto elem : cellMapping)
			for (auto id : getNeighborsIDs(elem.first))
				elem.second->neighbors.push_back(cellMapping[id]);
	}

	std::vector<std::string> getNeighborsIDs(const std::string& id)
	{
		char letter = id[0];
		int number = std::stoi(id.substr(1, 1));

		std::vector<std::string> neighbors;

		std::vector<std::string> neighborKeys = { "  ", "  ", "  ", "  ", "  ", "  " };
		neighborKeys[0][0] = char(letter - 1); neighborKeys[0][1] = char(48 + number);
		neighborKeys[1][0] = char(letter - 1); neighborKeys[1][1] = char(48 + number + 1);
		neighborKeys[2][0] = char(letter - 0); neighborKeys[2][1] = char(48 + number - 1);
		neighborKeys[3][0] = char(letter - 0); neighborKeys[3][1] = char(48 + number + 1);
		neighborKeys[4][0] = char(letter + 1); neighborKeys[4][1] = char(48 + number - 1);
		neighborKeys[5][0] = char(letter + 1); neighborKeys[5][1] = char(48 + number);

		for (auto cellID : allowedMoves)
		{
			if ((std::find(neighborKeys.begin(), neighborKeys.end(), cellID) != neighborKeys.end()))
				neighbors.push_back(cellID);
		}

		return neighbors;
	}
};

/////////////////////////////////////////////////////////////////////////////
//
// Random player
//
/////////////////////////////////////////////////////////////////////////////

class RandomPlayer : public BasePlayer
{
public:
	Move nextMove()
	{
		std::string cellID = allowedMoves[rand() % allowedMoves.size()];
		int cellValue = allowedValues[rand() % allowedValues.size()];
		allowedMoves.erase(std::remove(allowedMoves.begin(), allowedMoves.end(), cellID), allowedMoves.end());
		allowedValues.erase(std::remove(allowedValues.begin(), allowedValues.end(), cellValue), allowedValues.end());
		--movesLeft;
		return Move(cellID, cellValue);
	}
};

/////////////////////////////////////////////////////////////////////////////
//
// Specialization of player
//
/////////////////////////////////////////////////////////////////////////////

class Competitor : public BasePlayer
{
public:
	Move nextMove()
	{
		int bestScore = -9000;
		std::string bestCellID = allowedMoves[0];
		int maxValueToken = allowedValues[allowedValues.size() - 1];

		cout << "Loosing cells are: ";
		for (auto cellID : allowedMoves)
		{
			Cell *cell = cellMapping[cellID];
			if (cell->compoundValue < 0) cout << cellID << " ";
		}
		cout << endl;

		for (auto cellID : allowedMoves)
		{
			std::vector<std::string> neighbors = getNeighborsIDs(cellID);
			int cellScore = 0;
			bool loosingCell = false;
			for (auto neighborIDs : neighbors)
			{
				Cell *cell = cellMapping[neighborIDs];
				if (cell->cellType == PLAYABLE)
				{
					loosingCell = false;
					if((cell->compoundValue + maxValueToken) >= 0)
						cellScore += 1;
				}
			}

			if (loosingCell)
			{
				bestCellID = cellID;
				break;
			}

			cout << cellID << " has a score of " << cellScore << endl;
			if (cellScore > bestScore)
			{
				bestCellID = cellID;
				bestScore = cellScore;
			}
		}

		std::string cellID = bestCellID;
		int cellValue = allowedValues[allowedValues.size() - 1];
		allowedMoves.erase(std::remove(allowedMoves.begin(), allowedMoves.end(), cellID), allowedMoves.end());
		allowedValues.erase(std::remove(allowedValues.begin(), allowedValues.end(), cellValue), allowedValues.end());
		--movesLeft;

		std::vector<std::string> neighbors = getNeighborsIDs(cellID);
		for (auto neighborIDs : neighbors)
		{
			Cell *cell = cellMapping[neighborIDs];
			if (cell->cellType == CELL_TYPE::PLAYABLE) cell->compoundValue += cellValue;
		}

		return Move(cellID, cellValue);
	}
};

std::string receiveData(const SOCKET& socket, const int& len)
{
	char *inBuf = new char[len + 1];
	memset(inBuf, 0, len + 1);
	recv(socket, inBuf, len, 0);
	std::string in = std::string(inBuf);
	cout << "Received data: " << in << endl;
	return in;
}

void sendData(const SOCKET& socket, const std::string& out)
{
	cout << "Send data: " << out << endl;
	send(socket, out.c_str(), out.size(), 0);
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

	getaddrinfo("localhost", "6666", &hints, &result);
	srand(time(NULL));

	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
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

	BasePlayer *player = new Competitor();

	int nrGames = std::stoi(receiveData(ConnectSocket, 10));
	for (int i = 0; i < nrGames; ++i)
	{
		player->resetGame();
		int blockedCells = 5;
		while (blockedCells--) player->processBlockedMove(BasePlayer::raw2Move(receiveData(ConnectSocket, 2)));
		std::string move = receiveData(ConnectSocket, 5);
		while (move != "Quit")
		{
			player->processEnemyMove(BasePlayer::raw2Move(move));
			sendData(ConnectSocket, BasePlayer::move2Raw(player->nextMove()));
			move = receiveData(ConnectSocket, 5);
		}
	}

	closesocket(ConnectSocket);
	WSACleanup();

	return 0;
}