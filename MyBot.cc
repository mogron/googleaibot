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
  return (from->distance(protector) < from->distance(protectee) && 2*protectee->distance(protector) <= from->distance(protectee));
}

bool MyBot::protects(Planet* protector, Planet* protectee)
{
  Planets closest = protectee->closestPlanets();
  int dist = protector->distance(protectee);
  for(Planets::const_iterator pit = closest.begin(); pit != closest.end(); ++pit){
    Planet* p = *pit;
    if(p->distance(protectee) > 2 * dist){
      return true;
    }
    if(p->owner() != protectee->owner() && !p->owner()->isNeutral() && !protects(protector, protectee, p)){
      return false;
    }
  }
  return true;
}

bool MyBot::isProtectedFrom(Planet* pl, Planet* from)
{
  Planets closest = pl->closestPlanets();
  int dist = from->distance(pl);
  for(Planets::const_iterator pit = closest.begin(); pit != closest.end(); ++pit){
    Planet* p = *pit;
    if(p->owner() == pl->owner() && protects(p, pl, from)){
      return true;
    }
    if(p->distance(pl) > dist){
      return false;
    }
  }
  return false;
}


bool MyBot::isProtected(Planet* pl)
{
  Planets closest = pl->closestPlanets();
  for(Planets::const_iterator pit = closest.begin(); pit != closest.end(); ++pit){
    Planet* p = *pit;
    if (p->owner() != pl->owner() && !p->owner()->isNeutral()){
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

//TODO: get rid of code duplication with competitiveFleets()
vector<Fleet> MyBot::worstCaseFleets(Planet* pl)
{
  vector<Fleet> fs;
  Planets closest = pl->closestPlanets();
  for (Planets::const_iterator pit = closest.begin(); pit != closest.end(); ++pit) {
    Planet* p = *pit;

    int dist = p->distance(pl);
    int i(0);
    for ( vector<Planet>::iterator pFuture = predictions[p].begin(); pFuture != predictions[p].end(); ++pFuture){
      if(pFuture->owner()->isEnemy()){
        int sc;
        //temporary, not correct! (but fast)
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
 

int MyBot::willHoldFor(const vector<Planet>& predictions,int t)
{
  for ( vector<Planet>::const_iterator pFuture = predictions.begin()+t; pFuture != predictions.end(); ++pFuture){
    if(pFuture->owner()->isEnemy()) return pFuture - (predictions.begin()+t);
  }
  return turnsRemaining;
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




int MyBot::supplyMove(Planet* pl, Planet* goal){
	AStarSearch<Planet> astarsearch;
  Planet* ret;

  // Create a start state
  Planet nodeStart(*pl);


  // Define the goal state
  Planet nodeEnd(*goal);
		
		
  astarsearch.SetStartAndGoalStates( nodeStart, nodeEnd );

  unsigned int SearchState;
  unsigned int SearchSteps = 0;

  do
		{
			SearchState = astarsearch.SearchStep();

			SearchSteps++;

		}
  while( SearchState == AStarSearch<Planet>::SEARCH_STATE_SEARCHING );


  if( SearchState == AStarSearch<Planet>::SEARCH_STATE_SUCCEEDED )
		{

      Planet *node = astarsearch.GetSolutionStart();

      ret = astarsearch.GetSolutionNext();

      astarsearch.FreeSolutionNodes();
      astarsearch.EnsureMemoryFreed();
      if(ret){
        return ret->planetID();
      } else {
      return pl->planetID();
      }
	
		}
  else if( SearchState == AStarSearch<Planet>::SEARCH_STATE_FAILED ) 
		{
      astarsearch.EnsureMemoryFreed();
			return pl->planetID();
		}



  astarsearch.EnsureMemoryFreed();

	return pl->planetID();
}

int MyBot::myPredictedGrowthRate(int t)
{
  int gr(0);
  for(vector<Planet*>::const_iterator pit = planets.begin();pit!=planets.end();++pit){
    Planet* p = *pit;
    Planet pFut = predictions[p][t];
    if(pFut.owner()->isMe()){
      gr += pFut.growthRate();
    }
  }
  return gr;
}

int MyBot::enemyPredictedGrowthRate(int t)
{
  int gr(0);
  for(vector<Planet*>::const_iterator pit = planets.begin();pit!=planets.end();++pit){
    Planet* p = *pit;
    Planet pFut = predictions[p][t];
    if(pFut.owner()->isEnemy()){
      gr += pFut.growthRate();
    }
  }
  return gr;
}
      
  

void MyBot::executeTurn()
{
  cerr << "Turn: " << game->turn() << endl;
  const int turnLimit = 200;
  turnsRemaining = turnLimit - game->turn();
  planets = game->planets();
  const Planets myPlanets = game->myPlanets();
  const Planets notMyPlanets = game->notMyPlanets();
  const Planets enemyPlanets = game->enemyPlanets();
  if (game->turn() <= 1){
    initialize();
  }
  if(myPlanets.size() == 0 || enemyPlanets.size() ==0){ return;}

  Player* enemy = game->playerByID(2);
  Player* me = game->playerByID(1);
  const int lookahead = max_distance_between_planets;
  int enemy_sc = enemy->shipsCount();
  int me_sc = me->shipsCount();
  int my_growthRate = me->growthRate();
  int enemy_growthRate = enemy->growthRate();


  predictions.clear();
  competitivePredictions.clear();
  worstCasePredictions.clear();
  frontierStatus.clear();
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
    worstCasePredictions[p] = p->getPredictions(lookahead, worstCaseFleets(p));
  }

  for(vector<Planet*>::const_iterator pit = planets.begin();pit!=planets.end();++pit){
    Planet* p = *pit;
    frontierStatus[p] = isFrontier(p);
  }

  int my_predictedGrowthRate = myPredictedGrowthRate(lookahead); 
  int enemy_predictedGrowthRate = enemyPredictedGrowthRate(lookahead);

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
          if (shipsAvailableStatic < 0 && predictions[source][1].owner()->isEnemy()){
            shipsAvailableStatic = source->shipsCount();
          }
          if(max(shipsAvailableStatic, shipsAvailableCompetitive)>0){
            Planet futureDestination = predictions[destination][dist];
            int shipsRequired = futureDestination.shipsCount()+1;
            int shipsRequiredWorstCase = worstCasePredictions[destination][dist].shipsCount()+1;
            bool valid =  shipsRequired <= max(shipsAvailableCompetitive, shipsAvailableStatic) 
              &&!(futureDestination.owner()->isNeutral() 
                  && me_sc*2 < enemy_sc  ) 
              && !(futureDestination.owner()->isEnemy() 
                   && predictions[destination][dist-1].owner()->isNeutral()) 
              && (competitivePredictions[destination][dist].owner()->isMe() 
                  || (my_growthRate < enemy_growthRate 
                      && my_predictedGrowthRate < enemy_predictedGrowthRate));
            if(valid){
              Order o2(source, destination, max(shipsAvailableStatic,shipsAvailableCompetitive));
              orderCandidates.push_back(o2);
              Order o3(source, destination, min(shipsAvailableStatic, shipsAvailableCompetitive));
              orderCandidates.push_back(o3);
              if(!protects(destination, source)){
                Order o1(source, destination, shipsRequired); 
                orderCandidates.push_back(o1);       
              }   
              if(shipsRequiredWorstCase <= min(shipsAvailableStatic, shipsAvailableCompetitive)){
                Order o4(source, destination, shipsRequiredWorstCase);
                orderCandidates.push_back(o4);
              }
            }
          }
        }
    }

    int maxValue = 0;
    Order* maxOrder;
    for(Orders::iterator oit = orderCandidates.begin(); oit != orderCandidates.end(); ++oit){
      Planet* source = oit->sourcePlanet;
      Planet* destination = oit->destinationPlanet;
      int dist = source->distance(destination);
      int baseValue = value(predictions[source])+value(predictions[destination]);
      vector<Fleet> fs;
      Fleet f(myPlanets[0]->owner(), source, destination, oit->shipsCount, dist, dist);
      fs.push_back(f);
      if(predictions[destination][dist].owner()->isNeutral()){
        vector<Fleet> wfsSource = worstCaseFleets(source);
        vector<Fleet> wfsDest = worstCaseFleets(destination);
        wfsSource.push_back(f);
        wfsDest.push_back(f);
        if (willHoldFor(destination->getPredictions(lookahead,wfsDest), dist)*destination->growthRate() < destination->shipsCount()){
          continue;
        }
      }
      vector<Planet> sourcePredictions = source->getPredictions(lookahead, fs);
      vector<Planet> destinationPredictions = destination->getPredictions(lookahead, fs);
      int newValue = value(sourcePredictions)+value(destinationPredictions) - baseValue;
      if(newValue>maxValue){
        maxValue = newValue;
        maxOrder = &*oit; 
      }
    }
    if(maxValue>0){
      Planet* dest = maxOrder->destinationPlanet;
      Planet* source = maxOrder->sourcePlanet;
      game->issueOrder(*maxOrder);
      predictions[dest] = dest->getPredictions(lookahead);
      predictions[source] = source->getPredictions(lookahead);
      competitivePredictions[dest] = dest->getPredictions(lookahead, competitiveFleets(dest));
      competitivePredictions[source] = source->getPredictions(lookahead, competitiveFleets(dest));
    } 
  }
  
  for(Planets::const_iterator pit = myPlanets.begin(); pit != myPlanets.end(); ++pit){
    Planet* p = *pit;
    int shipsAvailable = p->shipsAvailable(predictions[p],lookahead);
    if(shipsAvailable > 0){
      Planet* target = game->planetByID(supplyMove(p, nearestFrontierPlanet(p)));
      Order o(p, target, shipsAvailable);
      game->issueOrder(o);
    }
  }
}
