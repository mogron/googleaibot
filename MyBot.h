#ifndef MINIBOT_H
#define MINIBOT_H

#include "abstractbot.h"

class MyBot : public AbstractBot
{
public:
    MyBot(Game* game);
    int max_distance_between_planets;
    void initialize();

    void executeTurn();
    static void log(char *format, ...);
};

#endif // MINIBOT_H
