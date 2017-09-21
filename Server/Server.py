from graphics import *
from enum import Enum
from pynput import keyboard
import math
import random
import socketserver
import socket
import sys
import datetime
import time
import _thread
import tkinter as tk
from tkinter import filedialog

RUN_IN_ONLINE_MODE = False
history = []
HISTORY_INDEX = -1

TIMESTAMP = None
RED_FOLDER = None
BLUE_FOLDER = None
DRAWS_FOLDER = None
GAME_NR = 0
CURRENT_FILE_NAME = None

WINDOW_SIZE_X = 800
WINDOW_SIZE_Y = 700

CELL_RADIUS = 50
DELAY = 0
NR_GAMES = 108

RED_COLOR = '#e43326'
BLUE_COLOR = '#2f41a5'
NORMAL_COLOR = '#dddddd'
BLOCKED_COLOR = '#694538'

window = GraphWin("CodeCup 2018 Server", WINDOW_SIZE_X, WINDOW_SIZE_Y)

#Server related messages
MESSAGE_START_GAME = 'Start'
MESSAGE_END_GAME = 'Quit'

cells = []
cellMap = {}

class CELL_TYPE(Enum):
    PLAYABLE = 0
    BLOCKED = 1
    RED_PLAYER = 2
    BLUE_PLAYER = 3

FIRST_PLAYER_COLOR = CELL_TYPE.RED_PLAYER
SECOND_PLAYER_COLOR = CELL_TYPE.BLUE_PLAYER

class Cell:

    def __init__(self, center, radius, type, letter):
        self.center = center
        self.radiu = radius
        self.type = type
        self.letter = letter

        vertices = []
        for i in range(0, 6):
            vertices.append(Point(center.x + radius * math.cos(2 * (math.pi * i / 6 + math.pi/12)), center.y + radius * math.sin(2 * (math.pi * i / 6 + math.pi/12))))

        self.label = Text(center, letter)
        self.label.setFace('courier')
        self.label.setSize(34)
        self.label.setWidth(60)
        
        self.shape = Polygon(vertices)
        self.shape.setFill(NORMAL_COLOR)
        self.shape.setWidth(2)

    def Draw(self, window):
        self.shape.draw(window)
        self.label.draw(window)

    def SetType(self, type):
        self.type = type
        if self.type == CELL_TYPE.PLAYABLE:
            self.shape.setFill(NORMAL_COLOR)
            self.label.setFill('black')
        elif self.type == CELL_TYPE.BLOCKED:
            self.shape.setFill(BLOCKED_COLOR)
            self.label.setFill('black')
        elif self.type == CELL_TYPE.RED_PLAYER:
            self.shape.setFill(RED_COLOR)
            self.label.setFill('#222222')
        elif self.type == CELL_TYPE.BLUE_PLAYER:
            self.shape.setFill(BLUE_COLOR)
            self.label.setFill('#f8f8f8')

    def GetType(self):
        return self.type

    def GetLetter(self):
        return self.letter

    def SetValue(self, value):
        self.value = value
        self.label.setText(str(value))

    def GetValue(self):
        return self.value

    def Reset(self):
        self.label.setText(self.letter)
        self.SetType(CELL_TYPE.PLAYABLE)

def readHistory():
    global history
    global FIRST_PLAYER_COLOR
    global SECOND_PLAYER_COLOR
    global HISTORY_INDEX
    
    clearBoard()
    clearScore()

    FIRST_PLAYER_COLOR = CELL_TYPE.RED_PLAYER
    SECOND_PLAYER_COLOR = CELL_TYPE.BLUE_PLAYER
    
    filePath = filedialog.askopenfilename(filetypes=(("Game files", "*.game"),))
    if filePath is '':
        return
    print('Selected file path: ' + filePath)
    
    with open(filePath) as f:
        history = f.readlines()
    history = [x.strip() for x in history]

    for h in history:
        print(h)

    for entry in history[:5]:
        cellMap[entry].SetType(CELL_TYPE.BLOCKED)

    if FIRST_PLAYER_COLOR.name != history[5]:
        FIRST_PLAYER_COLOR = CELL_TYPE.BLUE_PLAYER
        SECOND_PLAYER_COLOR = CELL_TYPE.RED_PLAYER

    print('First player: ' + FIRST_PLAYER_COLOR.name)
    print('Second player: ' + SECOND_PLAYER_COLOR.name)

    HISTORY_INDEX = 0

def moveFoward():
    global HISTORY_INDEX

    data = history[HISTORY_INDEX + 6]
    print(data)
    if HISTORY_INDEX % 2 == 0:
        cellMap[data[0:2]].SetType(FIRST_PLAYER_COLOR)
        cellMap[data[0:2]].SetValue(int(data[3:]))
    else:
        cellMap[data[0:2]].SetType(SECOND_PLAYER_COLOR)
        cellMap[data[0:2]].SetValue(int(data[3:]))

    HISTORY_INDEX += 1

def moveBackward():
    global HISTORY_INDEX

    data = history[HISTORY_INDEX + 5]
    print(data)
    cellMap[data[0:2]].SetType(CELL_TYPE.PLAYABLE)
    cellMap[data[0:2]].SetValue(data[0:2])

    HISTORY_INDEX -= 1

def appendToHistory(data):
    file = open(CURRENT_FILE_NAME, 'a')
    file.write(data + '\n')
    file.close()

def drawField():
    letter = 'A'
    fieldSize = 8

    startPosX = WINDOW_SIZE_X/2
    startPosY = CELL_RADIUS * 1.5

    lineSize = fieldSize + 1

    for i in range(1, lineSize):
        posX = startPosX
        posY = startPosY
        itemsToDraw = lineSize - i + 1
        for y in range(1, itemsToDraw):
            cell = Cell(Point(posX, posY), CELL_RADIUS, CELL_TYPE.PLAYABLE, letter + str(y))
            cells.append(cell)
            cellMap[letter + str(y)] = cell
            posX = posX + math.sqrt(3)/2.0 * CELL_RADIUS
            posY = posY + CELL_RADIUS * 1.5
        letter = str(chr(ord(letter) + 1))
        startPosX = startPosX - math.sqrt(3)/2.0 * CELL_RADIUS
        startPosY = startPosY + CELL_RADIUS * 1.5

    for cell in cells:
        cell.Draw(window)

def chooseRandomBlockedBlocks():
    for cell in cells:
        cell.SetType(CELL_TYPE.PLAYABLE)
    alreadyBlocked = []
    while len(alreadyBlocked) < 5:
        idx = random.randrange(0, len(cells), 1)
        if idx not in alreadyBlocked:
            alreadyBlocked.append(idx)
            cell = cells[idx]
            cell.SetType(CELL_TYPE.BLOCKED)
            appendToHistory(cell.GetLetter())

def sendDataToClient(conn, data):
    print('Sent data: ' + data);
    conn.send(bytes(data.encode()))

def readDataFromClient(conn):
    data = conn.recv(128)
    time.sleep(DELAY)
    dataRead = str(data, 'utf-8')
    print('Read data: ' + dataRead)
    appendToHistory(dataRead)
    return dataRead

def clearBoard():
    global GAME_NR
    global CURRENT_FILE_NAME
    
    CURRENT_FILE_NAME = 'GAME' + str(GAME_NR) + '.game'
    GAME_NR += 1
    for cell in cells:
        cell.Reset()

def getNeighbors(cellID):
    letter = cellID[0]
    number = int(cellID[1])

    neighbors = []

    neighborKeys = [(str(chr(ord(letter) - 1)) + str(number)),
                    (str(chr(ord(letter) - 1)) + str(number + 1)),
                    (str(chr(ord(letter) - 0)) + str(number - 1)),
                    (str(chr(ord(letter) - 0)) + str(number + 1)),
                    (str(chr(ord(letter) + 1)) + str(number - 1)),
                    (str(chr(ord(letter) + 1)) + str(number))]

    for key in cellMap.keys():
        if key in neighborKeys and cellMap[key].GetType() != CELL_TYPE.BLOCKED:
            neighbors.append(cellMap[key])

    return neighbors

redScore = 0
blueScore = 0
redTotalScore = 0
blueTotalScore = 0

label = Text(Point(WINDOW_SIZE_X * 0.80, 50), "Red - Blue")
label.setFace('courier')
label.setSize(24)
label.setWidth(WINDOW_SIZE_X)
label.draw(window)

generalScoreLabel = Text(Point(WINDOW_SIZE_X * 0.80, 90), "0 - 0")
generalScoreLabel.setFace('courier')
generalScoreLabel.setSize(24)
generalScoreLabel.setWidth(WINDOW_SIZE_X)
generalScoreLabel.draw(window)

victoryScoreLabel = Text(Point(WINDOW_SIZE_X * 0.80, 130), "0 - 0")
victoryScoreLabel.setFace('courier')
victoryScoreLabel.setSize(24)
victoryScoreLabel.setWidth(WINDOW_SIZE_X)
victoryScoreLabel.draw(window)

winnerLabel = Text(Point(WINDOW_SIZE_X * 0.22, 50), "")
scoreLabel = Text(Point(WINDOW_SIZE_X * 0.22, 90), "")
winnerLabel.setFace('courier')
scoreLabel.setFace('courier')
winnerLabel.setSize(36)
scoreLabel.setSize(36)
winnerLabel.setStyle('bold')
scoreLabel.setStyle('bold')
winnerLabel.setWidth(WINDOW_SIZE_X)
scoreLabel.setWidth(WINDOW_SIZE_X)

winnerLabel.draw(window)
scoreLabel.draw(window)

def clearScore():
    global redScore
    global blueScore
    global redTotalScore
    global blueTotalScore
    global generalScoreLabel
    global victoryScoreLabel
    global winnerLabel
    global scoreLabel

    redScore = 0
    blueScore = 0
    redTotalScore = 0
    blueTotalScore = 0
    
    generalScoreLabel.setText("0 - 0")
    victoryScoreLabel.setText("0 - 0")

    winnerLabel.setText("")
    scoreLabel.setText("")

def updateScoring():
    global redScore
    global blueScore
    global redTotalScore
    global blueTotalScore
    global winnerLabel
    global scoreLabel
    redPoints = 0
    bluePoints = 0

    for cell in cells:
        if cell.GetType() == CELL_TYPE.PLAYABLE:
            neighbors = getNeighbors(cell.GetLetter())
            for neighbor in neighbors:
                if neighbor.GetType() == CELL_TYPE.RED_PLAYER:
                    redPoints += neighbor.GetValue()
                elif neighbor.GetType() == CELL_TYPE.BLUE_PLAYER:
                    bluePoints += neighbor.GetValue()

    redFinalPoints = 75 - bluePoints + redPoints
    blueFinalPoints = 75 - redPoints + bluePoints

    redTotalScore += redFinalPoints
    blueTotalScore += blueFinalPoints
    
    if blueFinalPoints > redFinalPoints:
        blueScore += 1
        winnerLabel.setText("BLUE WON")
        winnerLabel.setFill(BLUE_COLOR)
        scoreLabel.setText(str(blueFinalPoints) + " to " + str(redFinalPoints))
        os.rename(CURRENT_FILE_NAME, BLUE_FOLDER + '/' + CURRENT_FILE_NAME)
    elif blueFinalPoints < redFinalPoints:
        redScore += 1
        winnerLabel.setText("RED WON")
        winnerLabel.setFill(RED_COLOR)
        scoreLabel.setText(str(redFinalPoints) + " to " + str(blueFinalPoints))
        os.rename(CURRENT_FILE_NAME, RED_FOLDER + '/' + CURRENT_FILE_NAME)
    elif blueFinalPoints == redFinalPoints:
        redTotalScore += redFinalPoints
        blueTotalScore += blueFinalPoints
        winnerLabel.setText("DRAW")
        winnerLabel.setFill('black')
        scoreLabel.setText(str(redFinalPoints) + " SAME")
        os.rename(CURRENT_FILE_NAME, DRAWS_FOLDER + '/' + CURRENT_FILE_NAME)

    generalScoreLabel.setText(str(redTotalScore) + ' - ' + str(blueTotalScore))
    victoryScoreLabel.setText(str(redScore) + ' - ' + str(blueScore))

def placeToken(token, value, color):
    if cellMap[token].GetType() == CELL_TYPE.PLAYABLE:
        cellMap[token].SetType(color)
        cellMap[token].SetValue(value)
    else:
        exit(-1)

def runServer():
    global FIRST_PLAYER_COLOR
    global SECOND_PLAYER_COLOR
    global RUN_IN_ONLINE_MODE
    global TIMESTAMP
    global RED_FOLDER
    global BLUE_FOLDER
    global DRAWS_FOLDER
    global GAME_NR
    global CURRENT_FILE_NAME
    global FIRST_PLAYER_COLOR
    global SECOND_PLAYER_COLOR

    TIMESTAMP = 'BATTLE_' + datetime.datetime.fromtimestamp(time.time()).strftime('%A_%H.%M.%S')
    RED_FOLDER = TIMESTAMP + '/RED_WINS'
    BLUE_FOLDER = TIMESTAMP + '/BLUE_WINS'
    DRAWS_FOLDER = TIMESTAMP + '/DRAWS'
    GAME_NR = 1
    CURRENT_FILE_NAME = None

    FIRST_PLAYER_COLOR = CELL_TYPE.RED_PLAYER
    SECOND_PLAYER_COLOR = CELL_TYPE.BLUE_PLAYER

    RUN_IN_ONLINE_MODE = True

    clearBoard()
    clearScore()
    
    os.makedirs(RED_FOLDER)
    os.makedirs(BLUE_FOLDER)
    
    redSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    redSocket.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
    redSocket.bind(('', 6666))
    redSocket.listen(1)

    blueSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    blueSocket.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
    blueSocket.bind(('', 6667))
    blueSocket.listen(1)

    print('Waiting for RED player')
    redConn, addr = redSocket.accept()

    sendDataToClient(redConn, str(NR_GAMES))
	
    print('Waiting for BLUE Player')
    blueConn, addr = blueSocket.accept()

    sendDataToClient(blueConn, str(NR_GAMES))

    for i in range(1, NR_GAMES + 1):
        clearBoard()
        chooseRandomBlockedBlocks()

        for cell in cells:
            if cell.GetType() == CELL_TYPE.BLOCKED:
                sendDataToClient(redConn, cell.GetLetter())
            
        for cell in cells:
            if cell.GetType() == CELL_TYPE.BLOCKED:
                sendDataToClient(blueConn, cell.GetLetter())

        firstPlayerConn = redConn
        secondPlayerConn = blueConn
        FIRST_PLAYER_COLOR = CELL_TYPE.RED_PLAYER
        SECOND_PLAYER_COLOR = CELL_TYPE.BLUE_PLAYER
        if i % 2 == 0:
            firstPlayerConn = blueConn
            secondPlayerConn = redConn
            FIRST_PLAYER_COLOR = CELL_TYPE.BLUE_PLAYER
            SECOND_PLAYER_COLOR = CELL_TYPE.RED_PLAYER

        sendDataToClient(firstPlayerConn, MESSAGE_START_GAME)
        appendToHistory(FIRST_PLAYER_COLOR.name)

        NR_MOVES = 15
        for i in range(0, NR_MOVES):                
            print('Waiting to read from ' + FIRST_PLAYER_COLOR.name)
            data = readDataFromClient(firstPlayerConn)
            print(data)
            placeToken(data[0:2], int(data[3:]), FIRST_PLAYER_COLOR)
            print('Sending move to ' + SECOND_PLAYER_COLOR.name)
            sendDataToClient(secondPlayerConn, data)
            print('Waiting to read from ' + SECOND_PLAYER_COLOR.name)
            data = readDataFromClient(secondPlayerConn)
            print(data)
            placeToken(data[0:2], int(data[3:]), SECOND_PLAYER_COLOR)
            if i != NR_MOVES - 1:
                print('Sending more to ' + FIRST_PLAYER_COLOR.name)
                sendDataToClient(firstPlayerConn, data)

        print("Sending QUIT to " + FIRST_PLAYER_COLOR.name)
        sendDataToClient(firstPlayerConn, MESSAGE_END_GAME)
        print("Sending QUIT to " + SECOND_PLAYER_COLOR.name)
        sendDataToClient(secondPlayerConn, MESSAGE_END_GAME)

        updateScoring()

    redSocket.close()
    blueSocket.close()

    RUN_IN_ONLINE_MODE = False
    print('Press s to start server, l to read file ...')

def on_press(key):
    try: k = key.char
    except: k = key.name
    print(k)
    if not RUN_IN_ONLINE_MODE:
        if k == 'right':
            if HISTORY_INDEX >= 0 and HISTORY_INDEX < 30:
                moveFoward()
        elif k == 'left':
            if HISTORY_INDEX > 0:
                moveBackward()
        elif k == 'l':
            readHistory()
        elif k == 's':
            _thread.start_new_thread( runServer, () )
        elif k == 'l':
            readHistory()

def main():
    print('Press s to start server, l to read file ...')
    window.mainloop()
    
if __name__ == "__main__":
    lis = keyboard.Listener(on_press=on_press)
    lis.start()
    drawField()
    main()
