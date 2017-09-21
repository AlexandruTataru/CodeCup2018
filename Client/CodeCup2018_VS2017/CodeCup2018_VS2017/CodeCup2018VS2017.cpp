#include "stdafx.h"

//#define CONSOLE_MODE
#define SOCKET_MODE
//#define OFFLINE_DEBUG_MODE
//#define PLAYER_COLOR "BLUE_PLAYER"

#ifdef SOCKET_MODE
	#define WIN32_LEAN_AND_MEAN

	#include <windows.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>

	#pragma comment (lib, "Ws2_32.lib")
	#pragma comment (lib, "Mswsock.lib")
	#pragma comment (lib, "AdvApi32.lib")
#endif

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <time.h>
#include <map>

using namespace std;

std::vector<std::string> offlineMoves;
size_t offlineMovesCursor = 0;

namespace CodeCup2018
{

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

	typedef std::string cellID;
	typedef int cellValue;
	typedef std::pair<cellID, cellValue> Move;

	/////////////////////////////////////////////////////////////////////////////
	//
	// Base player class with all the required logic
	//
	/////////////////////////////////////////////////////////////////////////////

	class BasePlayer
	{
	protected:
		std::vector<cellID> allowedMoves;
		std::vector<cellValue> allowedValues;
		std::vector<cellValue> enemyAllowedValues;
		std::map<cellID, Cell*> cellMapping;
		int movesLeft;
		bool movedFirst;
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
			cellID cellID = rawMove.substr(0, 2);
			cellValue cellValue = isPlayable ? std::stoi(rawMove.substr(3)) : 0;
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
			if (move.first == "St") 
			{
				movedFirst = true;
				return;
			}
			placeEnemyMove(move);
		}

		virtual Move nextMove() = 0;

		virtual void resetGame() {
			movedFirst = false;
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

		std::vector<cellID> getNeighborsIDs(const std::string& id)
		{
			char letter = id[0];
			int number = std::stoi(id.substr(1, 1));

			std::vector<cellID> neighbors;

			std::vector<cellID> neighborKeys = { "  ", "  ", "  ", "  ", "  ", "  " };
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

		std::vector<cellID> getLoosingCellsIDs()
		{
			std::vector<cellID> cells;
			for (auto cellID : allowedMoves) if (cellMapping[cellID]->compoundValue < 0) cells.push_back(cellID);
			return cells;
		}

		std::vector<cellID> getWinningCellsIDs()
		{
			std::vector<cellID> cells;
			for (auto cellID : allowedMoves) if (cellMapping[cellID]->compoundValue > 0) cells.push_back(cellID);
			return cells;
		}

		std::vector<cellID> getClearLoosingCellsIDs()
		{
			std::vector<cellID> clearlyLoosingCells;

			for (auto unplayedCellID : allowedMoves)
			{
				Cell *unplayedCell = cellMapping[unplayedCellID];
				bool hasPlayableNeighbors = std::find_if(unplayedCell->neighbors.begin(),
					unplayedCell->neighbors.end(), [this](Cell* cell) { return cell->cellType == PLAYABLE; }) != unplayedCell->neighbors.end();
				if(!hasPlayableNeighbors && unplayedCell->compoundValue < 0) clearlyLoosingCells.push_back(unplayedCellID);
			}

			return clearlyLoosingCells;
		}

		std::vector<cellID> getClearWinningCellsIDs()
		{
			std::vector<cellID> clearlyWinningCells;

			for (auto unplayedCellID : allowedMoves)
			{
				Cell *unplayedCell = cellMapping[unplayedCellID];
				size_t nrOfUnplayedSurroundingCells = 0;
				for (auto neighborCell : unplayedCell->neighbors) if (neighborCell->cellType == PLAYABLE) ++nrOfUnplayedSurroundingCells;
				int compoundValue = unplayedCell->compoundValue;
				if (nrOfUnplayedSurroundingCells)
				{
					size_t steps = min(nrOfUnplayedSurroundingCells, enemyAllowedValues.size());
					size_t highestIndex = enemyAllowedValues.size() - 1;
					compoundValue = unplayedCell->compoundValue;
					for (size_t i = 0; i < steps; ++i) compoundValue -= enemyAllowedValues[highestIndex - i];
				}

				if (compoundValue > 0) clearlyWinningCells.push_back(unplayedCellID);
			}

			return clearlyWinningCells;
		}

		void placeEnemyMove(const Move& enemyMove)
		{
			cellID id = enemyMove.first;
			cellValue cellValue = enemyMove.second;

			Cell *currentCell = cellMapping[id];

			currentCell->value = cellValue;
			currentCell->cellType = ENEMY;

			for (auto neighborCell : currentCell->neighbors) if (neighborCell->cellType == CELL_TYPE::PLAYABLE) neighborCell->compoundValue -= cellValue;

			allowedMoves.erase(std::remove(allowedMoves.begin(), allowedMoves.end(), id), allowedMoves.end());
			enemyAllowedValues.erase(std::remove(enemyAllowedValues.begin(),
				enemyAllowedValues.end(),
				cellValue),
				enemyAllowedValues.end());
		}

		void placeOwnMove(const Move& ownMove)
		{
			cellID id = ownMove.first;
			cellValue cellValue = ownMove.second;

			Cell *currentCell = cellMapping[id];

			allowedMoves.erase(std::remove(allowedMoves.begin(), allowedMoves.end(), id), allowedMoves.end());
			allowedValues.erase(std::remove(allowedValues.begin(), allowedValues.end(), cellValue), allowedValues.end());
			--movesLeft;

			for (auto neighborCell : currentCell->neighbors) if (neighborCell->cellType == CELL_TYPE::PLAYABLE) neighborCell->compoundValue += cellValue;

			currentCell->value = cellValue;
			currentCell->cellType = OWN;
		}

		void cancelMove(const Move& ownMove)
		{
			cellID id = ownMove.first;
			cellValue cellValue = ownMove.second;

			Cell *currentCell = cellMapping[id];

			allowedMoves.push_back(id);
			allowedValues.push_back(cellValue);

			for (auto neighborCell : currentCell->neighbors) if (neighborCell->cellType == CELL_TYPE::PLAYABLE) neighborCell->compoundValue -= cellValue;

			currentCell->cellType = PLAYABLE;
			currentCell->value = 0;

			++movesLeft;
		}

		bool isOpeningGame() { return movesLeft > 10; }

		bool isEndGame() { return movesLeft < 6; }

		bool isMiddleGame() { return !isOpeningGame() && !isEndGame(); }
	};

/////////////////////////////////////////////////////////////////////////////
//
// Random player
//
/////////////////////////////////////////////////////////////////////////////

	class RandomPlayer : public CodeCup2018::BasePlayer
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

	class Competitor : public CodeCup2018::BasePlayer
	{
		struct Gains
		{
			Move move;
			std::vector<cellID> wonCells;
			std::vector<cellID> lostCells;
			std::vector<cellID> winningCells;
			std::vector<cellID> loosingCells;

			std::string toString()
			{
				std::stringstream ss;
				ss << "Move: " << move.first << "=" << move.second;
				ss << " Won: "; for (auto cellID : wonCells) ss << cellID << " ";
				ss << " Lost: "; for (auto cellID : lostCells) ss << cellID << " ";
				ss << " Winning: "; for (auto cellID : winningCells) ss << cellID << " ";
				ss << " Loosing: "; for (auto cellID : loosingCells) ss << cellID << " ";
				return ss.str();
			}
		};

	public:

		Move nextMove()
		{
			std::vector<Gains> gains;
			std::vector<std::string> loosingCells = getClearLoosingCellsIDs();
			std::vector<std::string> winningCells = getClearWinningCellsIDs();

			auto aV = allowedValues;
			auto aM = allowedMoves;

			for (auto cellValue : aV)
			{
				for (auto cellID : aM)
				{
					Gains gain;
					placeOwnMove(Move(cellID, cellValue));
					gain.move = Move(cellID, cellValue);
					gain.wonCells = getClearWinningCellsIDs();
					gain.lostCells = getClearLoosingCellsIDs();
					gain.winningCells = getWinningCellsIDs();
					gain.loosingCells = getLoosingCellsIDs();
					cancelMove(Move(cellID, cellValue));
					gains.push_back(gain);
				}
			}

			
			int maxWon = 0, maxWinnings = 0;
			Move bestMove, nextBestMove;
			bool foundBestMove = false, foundNextBestMove = false;
			for (auto& gain : gains)
			{
				//cout << gain.toString() << endl;
				int actualWon = gain.wonCells.size();
				int actualWinnings = gain.winningCells.size();
				if (actualWon > maxWon)
				{
					maxWon = actualWon;
					bestMove = gain.move;
					foundBestMove = true;
				}
				if (actualWinnings > maxWinnings)
				{
					maxWinnings = actualWinnings;
					nextBestMove = gain.move;
					foundNextBestMove = true;
				}
			}

			Move choosenMove;
			if (maxWon != 0) choosenMove = bestMove;
			else choosenMove = nextBestMove;

			if (!foundBestMove && !foundNextBestMove)
			{
				std::string bestCellID = allowedMoves[rand() % allowedMoves.size()];
				int bestCellValue = allowedValues[rand() % allowedValues.size()];
				choosenMove = Move(bestCellID, bestCellValue);
			}

			placeOwnMove(choosenMove);

			return choosenMove;
		}
	};

}

#ifdef SOCKET_MODE
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
#elif defined(CONSOLE_MODE)
std::string receiveData()
{
	std::string data;
	cin >> data;
	return data;
}

void sendData(const std::string& out)
{
	cout << out << endl;
	cout.flush();
}
#elif defined(OFFLINE_DEBUG_MODE)
std::string receiveData()
{
	return offlineMoves.at(offlineMovesCursor++);
}

void sendData(const std::string& out) {}
#endif

int main(int argc, char **argv)
{
	using namespace CodeCup2018;
	BasePlayer *player = new Competitor();
	int nrGames = 1;

#ifdef SOCKET_MODE
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

	nrGames = std::stoi(receiveData(ConnectSocket, 10));
#elif defined(OFFLINE_DEBUG_MODE)
	std::ifstream infile("OfflineGame.txt");
	std::string line;
	while (std::getline(infile, line))
	{
		offlineMoves.push_back(line);
	}
	offlineMoves.erase(offlineMoves.begin() + 5);
#endif

	for (int i = 0; i < nrGames; ++i)
	{
		player->resetGame();
		int blockedCells = 5;
#ifdef SOCKET_MODE
		while (blockedCells--) player->processBlockedMove(BasePlayer::raw2Move(receiveData(ConnectSocket, 2)));
		std::string move = receiveData(ConnectSocket, 5);
#else
		while (blockedCells--) player->processBlockedMove(BasePlayer::raw2Move(receiveData()));
		std::string move = receiveData();
#endif
		while (move != "Quit")
		{
			player->processEnemyMove(BasePlayer::raw2Move(move));
#ifdef SOCKET_MODE
			sendData(ConnectSocket, BasePlayer::move2Raw(player->nextMove()));
			move = receiveData(ConnectSocket, 5);
#else
			sendData(BasePlayer::move2Raw(player->nextMove()));
			move = receiveData();
#endif
		}
	}

#ifdef SOCKET_MODE
	closesocket(ConnectSocket);
	WSACleanup();
#endif

	return 0;
}