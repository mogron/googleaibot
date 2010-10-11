#include "MyBot.h"

#include <stdio.h>
#include <stdarg.h>
#include <algorithm>
#include <map>
#include <vector>
#include <iostream>

#include "order.h"
#include "planet.h"
#include "player.h"

using std::min;
using std::max;
using std::vector;
using std::map;
using std::cerr;
using std::endl;

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

int MyBot::value(vector<Planet> predictions){
  int factor = 0;
  if(predictions.back().owner()->isMe()){
    factor = 1;
  } else if (predictions.back().owner()->isEnemy()){
    factor = -1;
  }
  return predictions.back().shipsCount() * factor;
}


bool MyBot::protects(Planet* protector, Planet* protectee, Planet* from)
{
  return (from->distance(protector) < from->distance(protectee) && 2*protectee->distance(protector) < from->distance(protectee));
}



bool MyBot::isProtected(Planet* pl)
{
  Planets closest = pl->closestPlanets();
  for(Planets::const_iterator pit = closest.begin(); pit != closest.end(); ++pit){
    Planet* p = *pit;
    if (p->owner()->isEnemy()){
      bool protectedFromp(false);
      for(Planets::const_iterator pit2 = closest.begin(); pit2 != closest.end(); ++pit2){
        Planet* p2 = *pit2;
        if(p2->owner()->isMe() && protects(p2,pl,p)){
          protectedFromp = true;
          break;
        }
      }
      if (!protectedFromp){
        return false;
      }
    }
  }
  return true;
}

bool MyBot::isFrontier(Planet* pl)
{
  Planets closest = pl->closestPlanets();
  for(Planets::iterator pit = closest.begin(); pit != closest.end(); ++pit){
    Planet* p = *pit;
    if (p->owner() != pl->owner() && !p->owner()->isNeutral()){
      Planets p_closest = p->closestPlanets();
      for(Planets::iterator pit2 = p_closest.begin(); pit2 != p_closest.end(); ++pit2){
        Planet* p2 = *pit2;
        if (p2->planetID() == pl->planetID()){
          return true;
        }
        if (p2->owner() == pl->owner()){
          return false;
        }
      }
    }
  }
}


Planet* MyBot::nearestFrontierPlanet(Planet* pl)
{
  if (frontierStatus[pl])
    return pl;
  Planets closest = pl->closestPlanets();
  for(Planets::iterator pit = closest.begin(); pit != closest.end(); ++pit){
    Planet* p = *pit;
    if (p->owner() == pl->owner() && frontierStatus[p])
      return p;
  }
  return pl;
}

int MyBot::distanceToFrontier(Planet* pl)
{
  return pl->distance(nearestFrontierPlanet(pl));
}

Planet* MyBot::nextPlanetCloserToFrontier(Planet* pl)
{
  if (frontierStatus[pl])
    return pl;                
  int dist = distanceToFrontier(pl);
  Planets closest = pl->closestPlanets();
  for(Planets::iterator pit = closest.begin(); pit != closest.end(); ++pit){
    Planet* p = *pit;
    if(p->owner() == pl->owner() && distanceToFrontier(p)<dist)
      return p;
  }
  return pl;
}



vector<Fleet> MyBot::competitiveFleets(Planet* pl)
{
  vector<Fleet> fs;
  Planets closest = pl->closestPlanets();
  for (Planets::const_iterator pit = closest.begin(); pit != closest.end(); ++pit) {
    Planet* p = *pit;

    int dist = p->distance(pl);
    int i(0);
    for ( vector<Planet>::iterator pFuture = predictions[p].begin(); pFuture != predictions[p].end(); ++pFuture){
      if(!pFuture->owner()->isNeutral()){
        int sc;
        //temporary, not correct!
        if(i == 0){
          sc = pFuture->shipsCount();
        } else {
          sc = pFuture->growthRate();
        } 
        Fleet f(pFuture->owner(), p, pl, sc, dist+i, dist+i);
        fs.push_back(f);
      }
      ++i;
    }
  }
  return fs;
}


Planets MyBot::knapsack01(Planets planets, int maxWeight) {
  vector<int> weights;
  vector<int> values;

  // solve 0-1 knapsack problem 
  for (Planets::const_iterator pit = planets.begin(); pit != planets.end(); ++pit) {
    Planet* p = *pit;
    // here weights and values are numShips and growthRate respectively
    // you can change this to something more complex if you like...
    weights.push_back(p->shipsCount()+1);
    values.push_back(p->growthRate()*(myStartingPlanet->distance(enemyStartingPlanet)-p->distance(myStartingPlanet)));
  }

  int K[weights.size()+1][maxWeight];

  for (int i = 0; i < maxWeight; i++) {
    K[0][i] = 0;
  }
  for (int k = 1; k <= weights.size(); k++) {
    for (int y = 1; y <= maxWeight; y++) {
      if (y < weights[k-1]){
        K[k][y-1] = K[k-1][y-1];
      }else if (y > weights[k-1]) {
        K[k][y-1] = max(K[k-1][y-1], K[k-1][y-1-weights[k-1]] + values[k-1]);
      } else {K[k][y-1] = max(K[k-1][y-1], values[k-1]);}
    }
  }

  // get the planets in the solution
  int i = weights.size();
  int currentW = maxWeight-1;
  Planets markedPlanets;

  while ((i > 0) && (currentW >= 0)) {
    if (((i == 0) && (K[i][currentW] > 0)) || (K[i][currentW] != K[i-1][currentW])) {
      markedPlanets.push_back(planets[i-1]);
      currentW = currentW - weights[i-1];
    }
    i--;
  }
  return markedPlanets;
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
  const Planets planets = game->planets();
  const Planets myPlanets = game->myPlanets();
  const Planets notMyPlanets = game->notMyPlanets();
  const Planets enemyPlanets = game->enemyPlanets(); 
  predictions.clear();
  competitivePredictions.clear();
  frontierStatus.clear();
  cerr << "Turn: " << game->turn() << endl;
  for(vector<Planet*>::const_iterator pit = planets.begin();pit!=planets.end();++pit){
    Planet* p = *pit;
    predictions[p] = p->getPredictions(lookahead);
  }

  for(vector<Planet*>::const_iterator pit = planets.begin();pit!=planets.end();++pit){
    Planet* p = *pit;
    competitivePredictions[p] = p->getPredictions(lookahead, competitiveFleets(p));
  }

  for(vector<Planet*>::const_iterator pit = planets.begin();pit!=planets.end();++pit){
    Planet* p = *pit;
    frontierStatus[p] = isFrontier(p);
  }

  // END OF PREPROCESSING //


  //Turn 1 Code //
  if(game->turn() == 1){
    myStartingPlanet = myPlanets[0];
    enemyStartingPlanet = enemyPlanets[0];
    Planets candidates;
    for(Planets::const_iterator pit = planets.begin(); pit != planets.end(); ++pit){
      Planet* p = *pit;
      if(p->distance(myStartingPlanet) < p->distance(enemyStartingPlanet)){
        candidates.push_back(p);
      }
    }
    
    int shipsAvailable = min(myStartingPlanet->shipsCount(), myStartingPlanet->growthRate() * myStartingPlanet->distance(enemyStartingPlanet));
      
    Planets targets = knapsack01(candidates,shipsAvailable);
    for(Planets::const_iterator pit = targets.begin(); pit != targets.end(); ++pit){
      Planet* p = *pit;
      Order o(myStartingPlanet, p, p->shipsCount()+1);
      game->issueOrder(o);
    }
    return;
  }

  //End of Turn 1 Code //

  const int maxActions = 10;

  for(int actions(0); actions != maxActions; ++actions){
    Orders orderCandidates;
    for(vector<Planet*>::const_iterator p = myPlanets.begin(); p!= myPlanets.end(); ++p){
      Planet* source = *p;
      int shipsAvailableStatic = source->shipsAvailable(predictions[source],lookahead);
        for(vector<Planet*>::const_iterator p = planets.begin(); p!= planets.end(); ++p){
          Planet* destination = *p;
          int dist = source->distance(destination);
          int shipsAvailableCompetitive = source->shipsAvailable(competitivePredictions[source], dist*2);
          int shipsAvailable = min(shipsAvailableCompetitive, shipsAvailableStatic);
          if (shipsAvailableCompetitive < 0 && shipsAvailableStatic < 0){
            shipsAvailable = source->shipsCount();
          }
          if(shipsAvailable>0){
            Planet futureDestination = predictions[destination][dist];
            int shipsRequired = futureDestination.shipsCount()+1;
            bool valid =  shipsRequired <= shipsAvailable && !(futureDestination.owner()->isNeutral() && me_sc*2 < enemy_sc ) && !(futureDestination.owner()->isEnemy() && predictions[destination][dist-1].owner()->isNeutral()) && competitivePredictions[destination][dist].owner()->isMe();
            if(valid){
              Order o1(source, destination, shipsRequired);
              Order o2(source, destination, shipsAvailable);
              orderCandidates.push_back(o1);
              orderCandidates.push_back(o2);
            }
          }
        }
    }

    int maxValue = 0;
    Order* maxOrder;
    for(Orders::iterator oit = orderCandidates.begin(); oit != orderCandidates.end(); ++oit){
      int dist = oit->sourcePlanet->distance(oit->destinationPlanet);
      int baseValue = value(predictions[oit->sourcePlanet])+value(predictions[oit->destinationPlanet]);
      vector<Fleet> fs;
      Fleet f(myPlanets[0]->owner(), oit->sourcePlanet, oit->destinationPlanet, oit->shipsCount, dist, dist);
      fs.push_back(f);
      vector<Planet> sourcePredictions = oit->sourcePlanet->getPredictions(lookahead, fs);
      vector<Planet> destinationPredictions = oit->destinationPlanet->getPredictions(lookahead, fs);
      int newValue = value(sourcePredictions)+value(destinationPredictions) - baseValue;
      if(newValue>maxValue){
        maxValue = newValue;
        maxOrder = &*oit; 
      }
    }
    if(maxValue>0){
      Planet* dest = maxOrder->destinationPlanet;
      game->issueOrder(*maxOrder);
      predictions[dest] = dest->getPredictions(lookahead);
      competitivePredictions[dest] = dest->getPredictions(lookahead, competitiveFleets(dest));
    } 
  }
  
  for(Planets::const_iterator pit = myPlanets.begin(); pit != myPlanets.end(); ++pit){
    Planet* p = *pit;
    int shipsAvailable = p->shipsAvailable(predictions[p],lookahead);
    if(shipsAvailable > 0){
      Order o(p, nextPlanetCloserToFrontier(p), shipsAvailable);
      game->issueOrder(o);
    }
  }
}
