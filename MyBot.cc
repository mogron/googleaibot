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
    
  for(std::vector<Planet*>::const_iterator p = myPlanets.begin(); p != myPlanets.end(); ++p)
      {
        Planet* sourcePlanet = *p;
        std::vector<Planet*> targets = sourcePlanet->closestPlanets();
        for(std::vector<Planet*>::const_iterator p2 = targets.begin(); p2 != targets.end();++p2){
          Planet* destinationPlanet = *p2;
          if(!destinationPlanet->owner()->isMe()){
        Order order = Order(sourcePlanet, destinationPlanet, sourcePlanet->shipsCount());
        game->issueOrder(order);
        break;
          }
        }
      }
}
