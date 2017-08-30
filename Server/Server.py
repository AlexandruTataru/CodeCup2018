from graphics import *
from enum import Enum
import math
import random
import socketserver
import socket
import sys
from _thread import *

WINDOW_SIZE_X = 800
WINDOW_SIZE_Y = 700
CELL_RADIUS = 50

window = GraphWin("CodeCup 2018 Server", WINDOW_SIZE_X, WINDOW_SIZE_Y)

class CELL_TYPE(Enum):
    PLAYABLE = 0
    BLOCKED = 1
6
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
        self.label.setSize(12)
        
        self.shape = Polygon(vertices)
        self.shape.setWidth(2)

    def Draw(self, window):
        self.shape.draw(window)
        self.label.draw(window)

    def SetType(self, type):
        self.type = type
        if self.type == CELL_TYPE.PLAYABLE:
            self.shape.setFill('lightgrey')
        elif self.type == CELL_TYPE.BLOCKED:
            self.shape.setFill('darkgrey')

    def GetType(self):
        return self.type

    def GetLetter(self):
        return self.letter

cells = []
cellMap = {}

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
    alreadyBlocked = []
    while len(alreadyBlocked) < 5:
        idx = random.randrange(0, len(cells), 1)
        if idx not in alreadyBlocked:
            alreadyBlocked.append(idx)
            cell = cells[idx]
            cell.SetType(CELL_TYPE.BLOCKED)

def sendDataToClient(conn, data):
    conn.send(bytes(data.encode()))

def readDataFromClient(sock, n):
    data = ''
    while len(data) < n:
        packet = sock.recv(n - len(data))
        data += str(packet)
    return data

def recvall(sock):
    BUFF_SIZE = 4096 # 4 KiB
    data = ""
    while True:
        part = sock.recv(BUFF_SIZE)
        data += str(part)
        if int(part) < BUFF_SIZE:
            # either 0 or end of data
            break
    return data

def main():
    redSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    redSocket.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
    redSocket.bind(('', 6666))
    redSocket.listen(10)

    blueSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    blueSocket.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
    blueSocket.bind(('', 6667))
    blueSocket.listen(10)

    print('Waiting for RED player')
    redConn, addr = redSocket.accept()

    for cell in cells:
        if cell.GetType() == CELL_TYPE.BLOCKED:
            sendDataToClient(redConn, cell.GetLetter())
    
    print('Waiting for BLUE Player')
    blueConn, addr = blueSocket.accept()

    for cell in cells:
        if cell.GetType() == CELL_TYPE.BLOCKED:
            sendDataToClient(blueConn, cell.GetLetter())

    sendDataToClient(redConn, "Start")
    data = recvall(redConn)
    print("Received data: " + str(data))

    redSocket.close()
    blueSocket.close()

if __name__ == "__main__":
    drawField()
    chooseRandomBlockedBlocks()
    main()
