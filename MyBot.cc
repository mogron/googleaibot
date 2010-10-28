/* 
   Author: Moritz Gronbach
   Oct2010
   Main file for my Planet Wars bot for the Google AI Challenge
   General Architecture:
   A filtered list of possible orders is created. Only targets with certain properties, depending on the situation, are allowed. The best order according to a simple heuristic is chosen (see value()). The evaluation of an order takes into account current fleet movements. 
   This process is repeated maxActions times.
   Once this is done, the ships that are currently on planets and not needed for defense are sent towards the frontline.
   The most critical areas for performance are:
   1. the decision what constitutes a valid target
   2. the degree of aggression in the frontier-planet selection 
   3. precision of calculating ships needed to conquer neutral planets

   TODO: -implement a panic()-function. When I have fewer ships than the enemy, and my production rate and predicted production rate is lower too, attack the enemy with full force.
   -make the prediction-architecture more efficient (fewer redundant calculations)
   -find a more elegant concept of target-validity
   -put utility-functions in separate class
*/

#include "MyBot.h"

#include <algorithm>
#include <map>
#include <vector>
#include <iostream>
#include <cmath>

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


//the entry function, this is called by the game engine
void MyBot::executeTurn() {
    cerr << "Turn: " << game->turn() << endl;
  
    if (game->turn() == 1) {
        initialize();
        preprocessing();
        firstTurn();
        return;
    }

    preprocessing();

    //this prevents timeouts caused by a certain bug in the game engine:
    if (myPlanets.size() == 0 || enemyPlanets.size() ==0) { 
        return;
    } 


    const int maxActions = 20;
    for(int actions(0); actions != maxActions; ++actions) {
        //find a good action and execute it, if possible
        if (!chooseAction()) { 
            break; //stop if no good action can be found
        }
    }

    //sent available ships towards the frontier
    supply();

    bool badSituation = me->shipsCount() < enemy->shipsCount() 
      && me->growthRate() < enemy->growthRate()
      && myPredictedGrowthRate(lookahead) < enemyPredictedGrowthRate(lookahead);
    if(badSituation){
      panic();
    }

    cerr << "Turn finished" << endl;
}


//this function is called only in turn 1, 
//and can be used for computing static properties of the map
void MyBot::initialize() {
    //calculate the size of the map, defined as the maximum distance between two planets
    maxDistanceBetweenPlanets = 1;
    const Planets planets = game->planets();
    for(Planets::const_iterator pit1 = planets.begin(); pit1!= planets.end();++pit1) {
        Planet* p1 = *pit1;
        for(Planets::const_iterator pit2 = planets.begin(); pit2!= planets.end();++pit2) {
            Planet* p2 = *pit2;
            int dist = p1->distance(p2);
            if (dist > maxDistanceBetweenPlanets) {
                maxDistanceBetweenPlanets = dist;
            }
        }
    }
    enemy = game->playerByID(2);
    me = game->playerByID(1);
}


//this updates the things that change from turn to turn, and makes predictions on planets future states
void MyBot::preprocessing() {
    const int turnLimit = 200; //according to the contest organizators, this will probably continue to be the turn limit until the end of the contest.
    turnsRemaining = turnLimit - game->turn();
    myPlanets = game->myPlanets();
    notMyPlanets = game->notMyPlanets();
    enemyPlanets = game->enemyPlanets();
    neutralPlanets = game->neutralPlanets();
    planets = game->planets();
    lookahead = maxDistanceBetweenPlanets;

    predictions.clear();
    competitivePredictions.clear();
    worstCasePredictions.clear();
    //static predictions taking into account only current fleet movements
    for(Planets::const_iterator pit = planets.begin();pit!=planets.end();++pit) {
        Planet* p = *pit;
        predictions[p] = p->getPredictions(lookahead);
    }

    //special-case predictions for each planet. It is important that the static predictions happen before this.
    for(Planets::const_iterator pit = planets.begin();pit!=planets.end();++pit) {
        Planet* p = *pit;
        worstCasePredictions[p] = p->getPredictions(lookahead, worstCaseFleets(p));
        competitivePredictions[p] = p->getPredictions(lookahead, competitiveFleets(p));
    }

    //determine which planets I will conquer according to current predictions.
    //These planets are candidates for frontier planets, even though I don't own them yet
    //TODO: this information should be updated immediately before computing supply lines
    for(Planets::const_iterator pit = planets.begin();pit!=planets.end();++pit) {
        Planet* p = *pit;
        p->predictedMine = false;
        for(vector<Planet>::const_iterator p2 = predictions[p].begin();p2!=predictions[p].end();++p2) {
            if (p2->owner()->isMe()) {
                p->predictedMine = true;
            }
        }
    }

    for(Planets::const_iterator pit = planets.begin();pit!=planets.end();++pit) {
        Planet* p = *pit;
        p->frontierStatus = isFrontier(p);
    }

    myPredictedGrowth = myPredictedGrowthRate(lookahead); 
    enemyPredictedGrowth = enemyPredictedGrowthRate(lookahead);
}


//this is executed in the first turn of the game. 
//Tries to allocate ships in a way that maximizes growth (by solving a knapsack01-problem), while not exposing the starting planet.
void MyBot::firstTurn() {
    myStartingPlanet = myPlanets[0];
    enemyStartingPlanet = enemyPlanets[0];
    Planets candidates;
    for(Planets::const_iterator pit = planets.begin(); pit != planets.end(); ++pit) {
        Planet* p = *pit;
        if (p->distance(myStartingPlanet) < p->distance(enemyStartingPlanet)) {
            candidates.push_back(p);
        }
    }
    
    int shipsAvailable = min(myStartingPlanet->shipsCount(), 
                             myStartingPlanet->growthRate() * myStartingPlanet->distance(enemyStartingPlanet));
      
    Planets targets = knapsack01(candidates,shipsAvailable);
    for(Planets::const_iterator pit = targets.begin(); pit != targets.end(); ++pit) {
        Planet* p = *pit;
        Order o(myStartingPlanet, p, p->shipsCount()+1);
        game->issueOrder(o);
    }
}


//tries to find a good action and executes it, returns true on success.
bool MyBot::chooseAction() {
    Orders orderCandidates;
    for(Planets::const_iterator p = myPlanets.begin(); p!= myPlanets.end(); ++p) {
        Planet* source = *p;
        addOrderCandidates(source, orderCandidates);
    }
  
    //find the best action candidate:
    int maxValue = 0;
    Order* maxOrder(0);
    for(Orders::iterator oit = orderCandidates.begin(); oit != orderCandidates.end(); ++oit) {
        Order* o=&*oit;
        int newValue = value(o);
        if (newValue>maxValue) {
            maxValue = newValue;
            maxOrder = o; 
        }
    }

    //execute the order with the highest value:
    if (maxValue>0 && maxOrder && maxOrder->isValid()) {
        Planet* dest = maxOrder->destinationPlanet;
        Planet* source = maxOrder->sourcePlanet;
        game->issueOrder(*maxOrder);
        //update the predictions that changed because of this order
        predictions[dest] = dest->getPredictions(lookahead);
        predictions[source] = source->getPredictions(lookahead);
        competitivePredictions[dest] = dest->getPredictions(lookahead, competitiveFleets(dest));
        competitivePredictions[source] = source->getPredictions(lookahead, competitiveFleets(dest));
        return true;
    } else {
    return false;
    }
}


//manages the supply chain
void MyBot::supply() {
    //find attractive neutral planets, so there will be sent ships towards them via the supply lines:
    setExpansionTargets();
  
    //compute available ships for each planet, i.e. ships that are not needed for defense according to the static predictions
    map<Planet*,int> shipsAvail;
    for(Planets::const_iterator pit = myPlanets.begin(); pit != myPlanets.end(); ++pit) {
        Planet* p = *pit;
        if (p->frontierStatus) {
            shipsAvail[p] = 0;
        } else {
            shipsAvail[p] = shipsAvailable(predictions[p],lookahead);
        }
    }
    
    //send available ships to the frontier
    for(Planets::const_iterator pit = myPlanets.begin(); pit != myPlanets.end(); ++pit) {
        Planet* p = *pit;
        if (shipsAvail[p]>0) {
            Planet* target = game->planetByID(supplyMove(p, nearestFrontierPlanet(p)));
            if (target->owner()->isMe() 
                || target->predictedMine 
                || (competitivePredictions[target][p->distance(target)].owner()->isMe())) {
                Order o(p, target, shipsAvail[p]);
                game->issueOrder(o);
            }
        }
    }
}


void MyBot::panic(){
  for(Planets::const_iterator pit = myPlanets.begin(); pit != myPlanets.end(); ++pit){
    Planet* p = *pit;
    if(p->frontierStatus){
      Planet* target = nearestEnemyPlanet(p);
      int dist = p->distance(target);
      Order o(p, target, shipsAvailable(predictions[p],dist * 2));
      game->issueOrder(o);
    }
  }
}

//find the order candidates for planet source. Orders are added by modifying the passed-by-ref orderCandidates.
//there is a lot of finetuning for special cases in this function, that makes it somewhat ugly.
void MyBot::addOrderCandidates(Planet* source, Orders& orderCandidates){
    int shipsAvailableStatic = shipsAvailable(predictions[source],lookahead);
    for(Planets::const_iterator p = planets.begin(); p!= planets.end(); ++p) {
        Planet* destination = *p;
        int dist = source->distance(destination);
        if(dist <= turnsRemaining){
          int shipsAvailableCompetitive = shipsAvailable(competitivePredictions[source], dist*2);
          if (shipsAvailableStatic < 0 && predictions[source][1].owner()->isEnemy()) {
            shipsAvailableStatic = source->shipsCount();
          }
          if (max(shipsAvailableStatic, shipsAvailableCompetitive)>0) {
            Planet futureDestination = predictions[destination][dist];
            int shipsRequired = futureDestination.shipsCount()+1;
            if (futureDestination.owner()->isMe()) {
              for(int t(dist); t!=lookahead+1; ++t) {
                if (predictions[destination][t].owner()->isEnemy()) {
                  shipsRequired = predictions[destination][t].shipsCount()+1;
                  break;
                }
              }
              continue;
            }
            int shipsRequiredWorstCase = worstCasePredictions[destination][dist].shipsCount()+1;
            //the conditions for validity make sure that my bot is not too aggressive, and does not attack neutrals when the enemy can snipe
            bool valid =  shipsRequired <= max(shipsAvailableCompetitive, shipsAvailableStatic) 
              &&!(futureDestination.owner()->isNeutral() 
                  && me->shipsCount()*2 < enemy->shipsCount()  ) 
              && !(futureDestination.owner()->isEnemy() 
                   && predictions[destination][dist-1].owner()->isNeutral()) 
              && (competitivePredictions[destination][dist].owner()->isMe() 
                  || competitivePredictions[destination][min(lookahead, 2*dist)].owner()->isMe()
                  || (me->growthRate() < enemy->growthRate() 
                      && myPredictedGrowth < enemyPredictedGrowth)
                  || me->shipsCount() > enemy->shipsCount() + shipsRequired);
            if (valid) {
              //add orders to the order candidates. 
              //Since there is time pressure I can not take all possible orders for this source and target, and instead try to cover the most important ones
              if (protects(source, destination)) {
                int sc = shipsRequired;
                orderCandidates.push_back(Order(source, destination, sc));
              } else if (source->frontierStatus) {
                if (protects(destination, source)) {
                  orderCandidates.push_back( Order(source, destination, shipsAvailableStatic));
                }
                orderCandidates.push_back(Order(source, destination, shipsAvailableCompetitive));
                if (shipsRequired < shipsAvailableCompetitive) {
                  orderCandidates.push_back(Order(source, destination, shipsAvailableCompetitive));
                }
                if (dist * 2 <= source->distance(enemyPlanets)) {
                  orderCandidates.push_back(Order(source, destination, shipsRequired));
                }
              } else {
                orderCandidates.push_back(Order(source, destination, max(shipsAvailableStatic,shipsAvailableCompetitive)));
                orderCandidates.push_back(Order(source, destination, min(shipsAvailableStatic, shipsAvailableCompetitive)));
                if (!protects(destination, source)) {
                  orderCandidates.push_back(Order(source, destination, shipsRequired));       
                }   
                if (shipsRequiredWorstCase <= min(shipsAvailableStatic, shipsAvailableCompetitive)) {
                  orderCandidates.push_back(Order (source, destination, shipsRequiredWorstCase));
                }
              }
            }
          }
        }
    }
}

int MyBot::value(Order* oit){
    Planet* source = oit->sourcePlanet;
    Planet* destination = oit->destinationPlanet;
    int dist = source->distance(destination);
    int baseValue = value(predictions[source]) + value(predictions[destination]);  //this is the value of doing no action
    vector<Fleet> fs;
    Fleet f(myPlanets[0]->owner(), source, destination, oit->shipsCount, dist, dist);
    fs.push_back(f);
    if (predictions[destination][dist].owner()->isNeutral()) {  //some tougher payoff conditions for neutral planets
        vector<Fleet> wfsSource = worstCaseFleets(source);
        vector<Fleet> wfsDest = worstCaseFleets(destination);
        wfsSource.push_back(f);
        wfsDest.push_back(f);
        //a neutral planet has to pay off even in the worst case:
        if (willHoldFor(destination->getPredictions(lookahead,wfsDest), dist)*destination->growthRate() < destination->shipsCount()) { 
            return 0;
        } 
    }
    vector<Planet> sourcePredictions = source->getPredictions(lookahead, fs);
    vector<Planet> destinationPredictions = destination->getPredictions(lookahead, fs);
    int newValue = value(sourcePredictions) + value(destinationPredictions) - baseValue;
    //higher distance means that the payoff predictions are less reliable, so closer targets are preferable:
    newValue *= (maxDistanceBetweenPlanets - dist + 1);  
    return newValue;
}


//find neutral planets that are interesting targets for expansion, and enemy planets that I might be able to conquer
//TODO: similar for enemy planets
void MyBot::setExpansionTargets(){
    //if there is a neutral planet that I might be able to conquer, and that pays off fast enough, make this neutral a frontier planet
    int fastestPayoff = lookahead;
    Planet* fastestPayoffPlanet(0);
    for(Planets::const_iterator pit = neutralPlanets.begin(); pit != neutralPlanets.end(); ++pit) {
        Planet* p = *pit;
        Planets temp = myPlanets;
        temp.push_back(p);
        if (p->growthRate()>0 
            && predictions[p][lookahead].owner()->isNeutral() 
            && competitivePredictions[p][lookahead].owner()->isMe() 
            && (me->shipsCount() 
                + me->growthRate() * distance(enemyPlanets,temp) 
                - predictions[p][lookahead].shipsCount() 
                >= enemy->shipsCount()) 
            && (me->growthRate() <= enemy->growthRate() 
                || myPredictedGrowth <= enemyPredictedGrowth)) {
            int timeToPayoff = predictions[p][lookahead].shipsCount() / p->growthRate() + 1;
            if (timeToPayoff < fastestPayoff) {
                fastestPayoff = timeToPayoff;
                fastestPayoffPlanet = p;
            }
        }
    }
    if (fastestPayoff < lookahead && fastestPayoffPlanet) {
        fastestPayoffPlanet->frontierStatus = true;
        for(Planets::const_iterator pit = planets.begin(); pit != planets.end(); ++pit) {
            Planet* p = *pit;
            if (p->planetID() != fastestPayoffPlanet->planetID() &&  protects(fastestPayoffPlanet, p)) {
                p->frontierStatus = false;
            }
        }
    }

    for(Planets::const_iterator pit = enemyPlanets.begin(); pit != enemyPlanets.end(); ++pit) {
      Planet* p = *pit;
      if(competitivePredictions[p][lookahead].owner()->isMe()){
        p->frontierStatus = true;
      }
      for(Planets::const_iterator pit2 = planets.begin(); pit2 != planets.end(); ++pit2) {
        Planet* p2 = *pit2;
        if (p2->planetID() != p->planetID() &&  protects(p, p2)) {
          p2->frontierStatus = false;
        }
      }
    }
}

//evaluates a list of predictions for a planet. This is used to calculate the value of an action/order.
int MyBot::value(const vector<Planet>& predictions) const{
    int factor = 0;
    Planet finalPlanet = predictions[min(lookahead,turnsRemaining)];
    if (finalPlanet.owner()->isMe()) {
        factor = 1;
    } else if (finalPlanet.owner()->isEnemy()) {
        factor = -1;
    }
    return finalPlanet.shipsCount() * factor;
}


//the following functions define the "protects" relationship and some utility function. 
//Planet A protects planet B from planet C, iff B can safely send ships to A, and A is closer to B than C. 
bool MyBot::protects(Planet* protector, Planet* protectee, Planet* from) const{
    return (from->distance(protector) < from->distance(protectee) 
            && 2*protectee->distance(protector) <= from->distance(protectee));
}


//Planet A protects planet B, if A protects B from all enemy planets.
bool MyBot::protects(Planet* protector, Planet* protectee) const{
    Planets closest = protectee->closestPlanets();
    int dist = protector->distance(protectee);
    for(Planets::const_iterator pit = closest.begin(); pit != closest.end(); ++pit) {
        Planet* p = *pit;
        if (p->distance(protectee) >= 2 * dist) {
            return true;
        }
        if (p->owner() != protectee->owner() 
            && !p->owner()->isNeutral() 
            && !protects(protector, protectee, p)) {
            return false;
        }
    }
    return true;
}

//returns true iff there is a planet that has the same owner as pl, and protects pl from Planet 'from'.
bool MyBot::isProtectedFrom(Planet* pl, Planet* from) const{
    Planets closest = pl->closestPlanets();
    int dist = from->distance(pl);
    for(Planets::const_iterator pit = closest.begin(); pit != closest.end(); ++pit) {
        Planet* p = *pit;
        if (p->owner() == pl->owner() && protects(p, pl, from)) {
            return true;
        }
        if (p->distance(pl) > dist) {
            return false;
        }
    }
    return false;
}


//returns true iff planet pl is protected from all enemy planets
bool MyBot::isProtected(Planet* pl) const{
    Planets closest = pl->closestPlanets();
    for(Planets::const_iterator pit = closest.begin(); pit != closest.end(); ++pit) {
        Planet* p = *pit;
        if (p->owner() != pl->owner() && !p->owner()->isNeutral()) {
            bool protectedFromp(false);
            for(Planets::const_iterator pit2 = closest.begin(); pit2 != closest.end(); ++pit2) {
                Planet* p2 = *pit2;
                if (p2->owner()->isMe() && protects(p2,pl,p)) {
                    protectedFromp = true;
                    break;
                }
            }
            if (!protectedFromp) {
                return false;
            }
        }
    }
    return true;
}


//determines if planet pl is one of my frontier planets.
//Simplified, pl is a frontier planet, if there is an enemy planet e, such that pl minimizes the distance from my planets to e.
bool MyBot::isFrontier(Planet* pl) const{
    if (!(pl->owner()->isMe() || pl->predictedMine)) {
        return false;
    }
    Planets closest = pl->closestPlanets();
    for(Planets::iterator pit = closest.begin(); pit != closest.end(); ++pit) {
        Planet* p = *pit;
        if (p->owner()->isEnemy() && !p->predictedMine) {
            Planets p_closest = p->closestPlanets();
            for(Planets::iterator pit2 = p_closest.begin(); pit2 != p_closest.end(); ++pit2) {
                Planet* p2 = *pit2;
                if (p2->planetID() == pl->planetID()) {
                    return true;
                }
                if ((p2->owner()->isMe()  || p2->predictedMine) && p2->distance(pl) < pl->distance(p)) {
                    return false;
                }
            }
        }
    }
    return false;
}


//returns my next frontier planet
Planet* MyBot::nearestFrontierPlanet(Planet* pl) const{
    if (pl->frontierStatus)
        return pl;
    Planets closest = pl->closestPlanets();
    for(Planets::iterator pit = closest.begin(); pit != closest.end(); ++pit) {
        Planet* p = *pit;
        if (p->frontierStatus)
            return p;
    }
    return pl;
}


Planet* MyBot::nearestEnemyPlanet(Planet* pl) const{
  if(pl->owner()->isEnemy()){
    return pl;
  }
  Planets closest = pl->closestPlanets();
  for(Planets::iterator pit = closest.begin(); pit != closest.end(); ++pit) {
    Planet* p = *pit;
    if (p->owner()->isEnemy())
      return p;
  }
  return pl;
}

//how far is planet pl away from my frontier?
int MyBot::distanceToFrontier(Planet* pl) const{
    return pl->distance(nearestFrontierPlanet(pl));
}


//what's the closest planet to pl that is closer to the frontier than pl?
Planet* MyBot::nextPlanetCloserToFrontier(Planet* pl) const{
    if (pl->frontierStatus)
        return pl;                
    int dist = distanceToFrontier(pl);
    Planets closest = pl->closestPlanets();
    for(Planets::iterator pit = closest.begin(); pit != closest.end(); ++pit) {
        Planet* p = *pit;
        if (p->owner() == pl->owner() && distanceToFrontier(p)<dist)
            return p;
    }
    return pl;
}

//computes the minimum distance between two sets of planets. 
int MyBot::distance(const Planets& ps1, const Planets& ps2) const{
    int dist = ps1[0]->distance(ps2[0]);
    for(Planets::const_iterator pit = ps1.begin(); pit != ps1.end(); ++pit) {
        Planet* p = *pit;
        dist = min(p->distance(ps2), dist);
    }
    return dist;
}

//this computes the fleets for the scenario where both players send all their ships to planet pl.
//Necessary for predicting if it is possible for me to conquer the planet
vector<Fleet> MyBot::competitiveFleets(Planet* pl) {
    vector<Fleet> fs;
    Planets closest = pl->closestPlanets();
    for (Planets::const_iterator pit = closest.begin(); pit != closest.end(); ++pit) {
        Planet* p = *pit;
        int dist = p->distance(pl);
        int i(0);
        int sent(0);
        for ( vector<Planet>::iterator pFuture = predictions[p].begin(); pFuture != predictions[p].end(); ++pFuture) {
            if (!pFuture->owner()->isNeutral()) {
                int sc;
                if (i == 0) {
                    sc = pFuture->shipsCount();
                } else {
                    sc = pFuture->shipsCount()-sent;
                } 
                if (sc>0) {
                    sent += sc;
                    Fleet f(pFuture->owner(), p, pl, sc, dist+i, dist+i);
                    fs.push_back(f);
                }
            }
            ++i;
        }
    }
    return fs;
}


//this computes the fleets in the scenario where the enemy sends all his ships to planet pl, and I send none.
//TODO: get rid of code duplication with competitiveFleets()
vector<Fleet> MyBot::worstCaseFleets(Planet* pl) {
    vector<Fleet> fs;
    Planets closest = pl->closestPlanets();
    for (Planets::const_iterator pit = closest.begin(); pit != closest.end(); ++pit) {
        Planet* p = *pit;

        int dist = p->distance(pl);
        int i(0);
        int sent(0);
        for ( vector<Planet>::iterator pFuture = predictions[p].begin(); pFuture != predictions[p].end(); ++pFuture) {
            if (pFuture->owner()->isEnemy()) {
                int sc;
                if (i == 0) {
                    sc = pFuture->shipsCount();
                } else {
                    sc = pFuture->shipsCount()-sent;
                } 
                if (sc>0) {
                    sent += sc;
                    Fleet f(pFuture->owner(), p, pl, sc, dist+i, dist+i);
                    fs.push_back(f);
                }
            }
            ++i;
        }
    }
    return fs;
}
 
//Given predictions of a planets future, compute how long I will hold this planet starting at time t. 
//This is used to check if conquering a neutral planet pays off in the worst case.
int MyBot::willHoldFor(const vector<Planet>& predictions,int t) const{
    for ( vector<Planet>::const_iterator pFuture = predictions.begin()+t; pFuture != predictions.end(); ++pFuture) {
        if (pFuture->owner()->isEnemy()) return pFuture - (predictions.begin()+t);
    }
    return turnsRemaining;
}
    

//A dynamic programming solution to the knapsack01-problem.
//This is used in the first turn to allocate ships in a way that maximizes the production rate.
Planets MyBot::knapsack01(const Planets& planets, int maxWeight) const{
    vector<int> weights;
    vector<int> values;

    for (Planets::const_iterator pit = planets.begin(); pit != planets.end(); ++pit) {
        Planet* p = *pit;
        weights.push_back(p->shipsCount()+1);
        values.push_back(p->growthRate()*(myStartingPlanet->distance(enemyStartingPlanet)-p->distance(myStartingPlanet)));
    }

    int K[weights.size()+1][maxWeight];

    for (int i = 0; i < maxWeight; i++) {
        K[0][i] = 0;
    }
    for (int k = 1; k <= weights.size(); k++) {
        for (int y = 1; y <= maxWeight; y++) {
            if (y < weights[k-1]) {
                K[k][y-1] = K[k-1][y-1];
            } else if (y > weights[k-1]) {
                K[k][y-1] = max(K[k-1][y-1], K[k-1][y-1-weights[k-1]] + values[k-1]);
            } else {K[k][y-1] = max(K[k-1][y-1], values[k-1]);}
        }
    }

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



//this computes the next planet in the supply chain starting in planet pl and ending in goal.
//I use A* for this, with dist*sqrt(dist) as cost function. This cost function favours smaller steps, which are needed for flexibility.
//The code for this function is mostly taken from http://code.google.com/p/a-star-algorithm-implementation/
int MyBot::supplyMove(Planet* pl, Planet* goal) {
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


    if ( SearchState == AStarSearch<Planet>::SEARCH_STATE_SUCCEEDED )
		{

        Planet *node = astarsearch.GetSolutionStart();

        ret = astarsearch.GetSolutionNext();

        astarsearch.FreeSolutionNodes();
        astarsearch.EnsureMemoryFreed();
        if (ret) {
            return ret->planetID();
        } else {
            return pl->planetID();
        }
	
		}
    else if ( SearchState == AStarSearch<Planet>::SEARCH_STATE_FAILED ) 
		{
        astarsearch.EnsureMemoryFreed();
        return pl->planetID();
		}



    astarsearch.EnsureMemoryFreed();

    return pl->planetID();
}


//computes my growth rate in t steps, given current fleet movements
int MyBot::myPredictedGrowthRate(int t) {
    int gr(0);
    for(Planets::const_iterator pit = planets.begin();pit!=planets.end();++pit) {
        Planet* p = *pit;
        Planet pFut = predictions[p][t];
        if (pFut.owner()->isMe()) {
            gr += pFut.growthRate();
        }
    }
    return gr;
}


int MyBot::enemyPredictedGrowthRate(int t) {
    int gr(0);
    for(Planets::const_iterator pit = planets.begin();pit!=planets.end();++pit) {
        Planet* p = *pit;
        Planet pFut = predictions[p][t];
        if (pFut.owner()->isEnemy()) {
            gr += pFut.growthRate();
        }
    }
    return gr;
}

int MyBot::shipsAvailable(const vector<Planet>& predictions, int t) const{
    int available = 100000;
    int i(0);
    for(std::vector<Planet>::const_iterator pit = predictions.begin(); pit != predictions.end(); ++pit) {
        int sc = pit->shipsCount();
        if (pit->owner()->isMe()) {
            available = min(available,sc);
        } else {
            available = min(available, -sc);
            break;
        }
        i++;
        if (i>t)break;
    }
    return available;
}
      

  

