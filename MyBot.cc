#include "MyBot.h"

#include <stdio.h>
#include <stdarg.h>

#include "order.h"
#include "planet.h"
#include "player.h"

MyBot::MyBot(Game* game) :
    AbstractBot(game)
{
}

void MyBot::executeTurn()
{
    const std::vector<Planet*> myPlanets = game->myPlanets();
    const std::vector<Planet*> notMyPlanets = game->notMyPlanets();

    Planet* sourcePlanet = myPlanets.at(0);
    Planet* destinationPlanet = notMyPlanets.at(0);
    Order order = Order(sourcePlanet, destinationPlanet, 1);

    game->issueOrder(order);
}
