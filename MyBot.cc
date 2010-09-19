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
  const int turnLimit = 200;
  const std::vector<Planet*> planets = game->planets();
  const std::vector<Planet*> myPlanets = game->myPlanets();
  const std::vector<Planet*> notMyPlanets = game->notMyPlanets();
    
  for(std::vector<Planet*>::const_iterator p = myPlanets.begin(); p != myPlanets.end(); ++p){
    Planet* sourcePlanet = *p;
    int minPayoffTime = turnLimit;
    Planet* minPayoffPlanet; 
    int minShipsNeeded;
    std::vector<Planet*> targets = notMyPlanets;
    for(std::vector<Planet*>::const_iterator p2 = targets.begin(); p2 != targets.end();++p2){
      Planet* destinationPlanet = *p2;
      int dist = sourcePlanet->distance(destinationPlanet);
      Planet futureDestinationPlanet = destinationPlanet->inFuture(dist);
      int payoffTime = futureDestinationPlanet.timeToPayoff() + dist;
      if(!futureDestinationPlanet.owner()->isMe() && futureDestinationPlanet.shipsCount() + 1 < sourcePlanet->shipsCount() && payoffTime < minPayoffTime){
        minPayoffTime = payoffTime;
        minPayoffPlanet = destinationPlanet;
        minShipsNeeded = futureDestinationPlanet.shipsCount()+1;
      }
    }
    if(minPayoffTime < turnLimit){
      Order order = Order(sourcePlanet, minPayoffPlanet,minShipsNeeded);
      game->issueOrder(order);
    }
  }
}
