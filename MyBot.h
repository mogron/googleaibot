#ifndef MINIBOT_H
#define MINIBOT_H

#include "abstractbot.h"

class MyBot : public AbstractBot
{
public:
    MyBot(Game* game);
    int max_distance_between_planets;
    void initialize();

    Planet* myStartingPlanet;
    Planet* enemyStartingPlanet;

    void executeTurn();
    static void log(char *format, ...);

 private:
    Planets knapsack01(Planets planets, int maxWeight);
};

#endif // MINIBOT_H
