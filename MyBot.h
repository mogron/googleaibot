#ifndef MINIBOT_H
#define MINIBOT_H

#include "abstractbot.h"

class MyBot : public AbstractBot
{
public:
    MyBot(Game* game);

    void executeTurn();
    static void log(char *format, ...);
};

#endif // MINIBOT_H
