#include <iostream>
#include "MyBot.h"

#include <sys/time.h>
#include <stdio.h>

// This is just the main game loop that takes care of communicating with the game engine for you.
int main() {
    Game game;
    MyBot myBot(&game);
    std::string currentLine;
    std::string mapData;
    while (true) {
        int c = std::cin.get();
        currentLine += (char)c;
        if (c == '\n') {
            if (currentLine.length() >= 2 && currentLine.substr(0, 2) == "go") {
                switch(game.turn()) {
                case 0:
                    game.initializeState(mapData);
                    break;
                default:
                    game.updateState(mapData);
                    break;
                }

                myBot.executeTurn();
                game.finishTurn();

                mapData = "";
            }
            else {
                mapData += currentLine;
            }
            currentLine = "";
        }
    }
    return 0;
}

