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


void MyBot::initialize(){
  //calculate the size of the map
  max_distance_between_planets = 1;
  const std::vector<Planet*> planets = game->planets();
  for(std::vector<Planet*>::const_iterator pit1 = planets.begin(); pit1!= planets.end();++pit1){
    Planet* p1 = *pit1;
      for(std::vector<Planet*>::const_iterator pit2 = planets.begin(); pit2!= planets.end();++pit2){
        Planet* p2 = *pit2;
        int dist = p1->distance(p2);
        if (dist > max_distance_between_planets){
          max_distance_between_planets = dist;
        }
      }
  }
}



void MyBot::executeTurn()
{
  if (game->turn() <= 1){
    initialize();
  }
  const int turnLimit = 200;
  const int lookahead = max_distance_between_planets;
  const std::vector<Planet*> planets = game->planets();
  const std::vector<Planet*> myPlanets = game->myPlanets();
  const std::vector<Planet*> notMyPlanets = game->notMyPlanets();
  for(std::vector<Planet*>::const_iterator pit = planets.begin();pit!=planets.end();++pit){
    Planet* p = *pit;
    p->updateFuture(lookahead);
  }


  for(std::vector<Planet*>::const_iterator p = myPlanets.begin(); p != myPlanets.end(); ++p){
    Planet* sourcePlanet = *p;
    int minPayoffTime = turnLimit;
    Planet* minPayoffPlanet; 
    int minShipsNeeded;
    std::vector<Planet*> targets = notMyPlanets;
    for(std::vector<Planet*>::const_iterator p2 = targets.begin(); p2 != targets.end();++p2){
      Planet* destinationPlanet = *p2;
      int dist = sourcePlanet->distance(destinationPlanet);
      Planet futureDestinationPlanet = destinationPlanet->getPredictions()[dist];
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
