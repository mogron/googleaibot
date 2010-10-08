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
  map<Planet*, vector<Planet > > predictions;
  for(vector<Planet*>::const_iterator pit = planets.begin();pit!=planets.end();++pit){
    Planet* p = *pit;
    predictions[p] = p->getPredictions(lookahead);
    p->updateFrontierStatus();
  }


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
      int shipsAvailable = source->shipsAvailable(predictions[source]);
      if (shipsAvailable < 0){
        shipsAvailable = source->shipsCount();
        }
      cerr << shipsAvailable << std::endl;
      for(vector<Planet*>::const_iterator p = planets.begin(); p!= planets.end(); ++p){
        Planet* destination = *p;
        int dist = source->distance(destination);
        Planet futureDestination = predictions[destination][dist];
        int shipsRequired = futureDestination.shipsCount()+1;
        bool valid = !futureDestination.owner()->isMe() && shipsRequired <= shipsAvailable && !(futureDestination.owner()->isNeutral() && me_sc < enemy_sc ) && !(futureDestination.owner()->isEnemy() && predictions[destination][dist-1].owner()->isNeutral()) ;
        if(valid){
          Order o1(source, destination, shipsRequired);
          Order o2(source, destination, shipsAvailable);
          orderCandidates.push_back(o1);
          orderCandidates.push_back(o2);
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
      game->issueOrder(*maxOrder);
    } 
  }
  
  for(Planets::const_iterator pit = myPlanets.begin(); pit != myPlanets.end(); ++pit){
    Planet* p = *pit;
    Order o(p, p->nextPlanetCloserToFrontier(), p->shipsAvailable(predictions[p]));
    game->issueOrder(o);
  }
}
