from graphics import *
from enum import Enum
import math
import random
import socketserver
import sys

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

class MyTCPHandler(socketserver.BaseRequestHandler):
    def handle(self):
        # self.request is the TCP socket connected to the client
        while True:
            self.data = self.request.recv(1024)
            print(self.data)
        # just send back the same data, but upper-cased
        #self.request.sendall(self.data.upper())

if __name__ == "__main__":
    drawField()
    chooseRandomBlockedBlocks()

    HOST, PORT = 'localhost', 4500
    server = socketserver.TCPServer((HOST, PORT), MyTCPHandler)
    server.serve_forever()
