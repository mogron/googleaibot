#include "MyBot.h"

#include <stdio.h>
#include <stdarg.h>
#include <algorithm>


#include "order.h"
#include "planet.h"
#include "player.h"

using std::min;
using std::vector;


MyBot::MyBot(Game* game) :
    AbstractBot(game)
{
}


void MyBot::initialize(){
  //calculate the size of the map
  max_distance_between_planets = 1;
  const vector<Planet*> planets = game->planets();
  for(vector<Planet*>::const_iterator pit1 = planets.begin(); pit1!= planets.end();++pit1){
    Planet* p1 = *pit1;
      for(vector<Planet*>::const_iterator pit2 = planets.begin(); pit2!= planets.end();++pit2){
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
  Player* enemy = game->playerByID(2);
  Player* me = game->playerByID(1);
  int enemy_sc = enemy->shipsCount();
  int me_sc = me->shipsCount();
  const int lookahead = max_distance_between_planets;
  const vector<Planet*> planets = game->planets();
  const vector<Planet*> myPlanets = game->myPlanets();
  const vector<Planet*> notMyPlanets = game->notMyPlanets();
  for(vector<Planet*>::const_iterator pit = planets.begin();pit!=planets.end();++pit){
    Planet* p = *pit;
    p->updateFuture(lookahead);
    p->updateFrontierStatus();
  }

  const int maxactions = 3;
  
  for(int actions(0); actions != maxactions; ++actions){
    for(vector<Planet*>::const_iterator p = myPlanets.begin(); p != myPlanets.end(); ++p){
      Planet* sourcePlanet = *p;
      
      int minPayoffTime = turnLimit;
      Planet* minPayoffPlanet; 
      int minShipsNeeded;

      int maxShipsAvailable = sourcePlanet->shipsAvailable();
      if (maxShipsAvailable < 0){
        maxShipsAvailable = sourcePlanet->shipsCount();
      }
      vector<Planet*> targets = planets;
      for(vector<Planet*>::const_iterator p2 = targets.begin(); p2 != targets.end();++p2){
        Planet* destinationPlanet = *p2;
        int dist = sourcePlanet->distance(destinationPlanet);
        Planet futureDestinationPlanet = destinationPlanet->getPredictions()[dist];
        int payoffTime = futureDestinationPlanet.timeToPayoff() + dist;
        bool valid = !futureDestinationPlanet.owner()->isMe() && futureDestinationPlanet.shipsCount() + 1 < maxShipsAvailable && payoffTime < minPayoffTime && !(futureDestinationPlanet.owner()->isNeutral() && me_sc < enemy_sc ) ;
        if(valid){
          minPayoffTime = payoffTime;
          minPayoffPlanet = destinationPlanet;
          minShipsNeeded = futureDestinationPlanet.shipsCount()+1;
        }
      }
      if(minPayoffTime < turnLimit){
        Order order = Order(sourcePlanet, minPayoffPlanet,maxShipsAvailable);
        game->issueOrder(order);
        sourcePlanet->updateFuture(lookahead);
        minPayoffPlanet->updateFuture(lookahead);
      } else if(!sourcePlanet->isFrontier()){
        Order order = Order(sourcePlanet, sourcePlanet->nextPlanetCloserToFrontier(), maxShipsAvailable/2);
        game->issueOrder(order);
        sourcePlanet->updateFuture(lookahead);
        sourcePlanet->nextPlanetCloserToFrontier()->updateFuture(lookahead);
      }
    }
  }
}
