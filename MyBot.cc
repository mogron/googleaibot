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

   TODO:
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
  AbstractBot(game),
  logging(true)
{
}


//the entry function, this is called by the game engine
void MyBot::executeTurn() {
  cerr << "Turn: " << game->turn() << endl;
  
  if (game->turn() == 1) {
    initialize();
    cerr << "MaxDistanceBetweenPlanets: " << maxDistanceBetweenPlanets << endl;
    preprocessing();
    firstTurn();
    return;
  }

  preprocessing();

  //this prevents timeouts caused by a certain bug in the game engine:
  if (myPlanets.size() == 0 || enemyPlanets.size() ==0) { 
    return;
  } 


  const int maxActions = 100;
  for(int actions(0); actions != maxActions; ++actions) {
    //find a good action and execute it, if possible
    if (!chooseAction()) { 
      break; //stop if no good action can be found
    }
  }

  updatePredictions();
  
  bool badSituation = me->shipsCount() < enemy->shipsCount()
    && game->turn() > maxDistanceBetweenPlanets
    && ((me->growthRate() < enemy->growthRate()
         && myPredictedGrowthRate(lookahead) < enemyPredictedGrowthRate(lookahead))
        || (turnsRemaining < maxDistanceBetweenPlanets*2
            && me->growthRate() <= enemy->growthRate()
            && myPredictedGrowthRate(lookahead) <= enemyPredictedGrowthRate(lookahead)));
  if(badSituation){
    panic();
    updatePredictions();
  }
  //sent available ships towards the frontier
  supply();



   
  bool dominating = me->shipsOnPlanets() > enemy->shipsCount()*1.1 && enemyPlanets.size()>0;
  if(dominating){
    updatePredictions(); 
    int maxShips(0);
    Planet* maxPlanet;
    for(Planets::iterator pit = myPlanets.begin(); pit != myPlanets.end(); ++pit){
      Planet* p = *pit;
      int sc = p->shipsCount();
      if(sc>maxShips){
        maxShips = sc;
        maxPlanet = p;
      }
    }
    if(maxShips>0){
      Order o(maxPlanet, nearestEnemyPlanet(maxPlanet), min(maxShips, me->shipsCount() - 11*enemy->shipsCount()/10)/2);
      issueOrder(o, "domination");
    }
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
  
  updatePredictions();
  for(Planets::const_iterator pit = myPlanets.begin();pit!=myPlanets.end();++pit) {
    Planet* p = *pit;
    cerr << p->planetID() << ": comp: " << shipsAvailable(competitivePredictions[p], lookahead) << "; static: " << shipsAvailable(predictions[p], lookahead) <<  endl;
  }

}


void MyBot::updatePredictions(){
  predictions.clear();
  competitivePredictions.clear();
  worstCasePredictions.clear();
  maxOutgoingFleets.clear();
  //static predictions taking into account only current fleet movements
  for(Planets::const_iterator pit = planets.begin();pit!=planets.end();++pit) {
    Planet* p = *pit;
    predictions[p] = p->getPredictions(lookahead);
  }

  for(Planets::const_iterator pit = planets.begin();pit!=planets.end();++pit) {
    Planet* p = *pit;
    maxOutgoingFleets[p] = computeMaxOutgoingFleets(p);
  }

  //special-case predictions for each planet. It is important that the static predictions happen before this.
  for(Planets::const_iterator pit = planets.begin();pit!=planets.end();++pit) {
    Planet* p = *pit;
    worstCasePredictions[p] = p->getPredictions(lookahead, worstCaseFleets(p));
    competitivePredictions[p] = p->getPredictions(lookahead, competitiveFleets(p));
  }


  //determine which planets I could conquer according to current predictions.
  //These planets are candidates for frontier planets, even though I don't own them yet
  //TODO: this information should be updated immediately before computing supply lines
  for(Planets::const_iterator pit = planets.begin();pit!=planets.end();++pit) {
    Planet* p = *pit;
    p->predictedMine = false;
    for(int i(0); i != lookahead + 1; ++i){
      if (predictions[p][i].owner()->isMe() && competitivePredictions[p][i].owner()->isMe()) {
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
    Order o(myStartingPlanet, p, p->shipsCount() + 1); 
    issueOrder(o, "first turn knapsack move.");
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
    issueOrder(*maxOrder, "action evaluation");
    updatePredictions();
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
          || target->predictedMine) {
        Order o(p, target, shipsAvail[p]);
        issueOrder(o, "supply");
      } else if(competitivePredictions[target][p->distance(target)].owner()->isMe() && !predictions[target][lookahead].owner()->isMe()){
        Order o(p, target, min(shipsAvail[p], predictions[target][lookahead].shipsCount() + 1));
        issueOrder(o, "supply");
      } else {
        cerr << "Planet " << p->planetID() << " did not supply his frontier planet" << target->planetID() << endl;
      }
    }
  }
}


void MyBot::panic(){
  for(Planets::const_iterator pit = myPlanets.begin(); pit != myPlanets.end(); ++pit){
    Planet* p = *pit;
    if(p->frontierStatus){
      Planets closest = p->closestPlanets();
      for(Planets::const_iterator pit2 = closest.begin(); pit2 != closest.end(); ++pit2){
        Planet* p2 = *pit2;
        if(p2->owner()->isEnemy()){
          int dist = p->distance(p2);
          int shipsRequired = predictions[p2][dist].shipsCount() + 1;
          if(p->shipsCount() >= shipsRequired){
            Order o(p, p2, shipsRequired);
            issueOrder(o, "panic");
          }
        }
      }
    }
  }
}

//find the order candidates for planet source. Orders are added by modifying the passed-by-ref orderCandidates.
//there is a lot of finetuning for special cases in this function, that makes it somewhat ugly.
void MyBot::addOrderCandidates(Planet* source, Orders& orderCandidates){
  int shipsAvailableStatic = shipsAvailable(predictions[source],lookahead);
  if (shipsAvailableStatic < 0 && willHoldFor(predictions[source], 0) < source->distance(nearestFriendlyPlanet(source))) {
    shipsAvailableStatic = source->shipsCount();
  }
  Planets sourceCluster = cluster(source);
  for(Planets::const_iterator p = planets.begin(); p!= planets.end(); ++p) {
    Planet* destination = *p;
    int dist = source->distance(destination);
    if(destination != source && dist <= turnsRemaining && dist <= maxDistanceBetweenPlanets){
      int shipsAvailableCompetitive = shipsAvailable(competitivePredictions[source], dist*2);
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
              && me->shipsCount()*2 < enemy->shipsCount() ) 
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
          if (protects(source, destination) || destination->owner()->isMe()) {
            orderCandidates.push_back(Order(source, destination, shipsRequired));
          } else if (source->frontierStatus) {
            if (protects(destination, source) && destination->owner()->isMe()) {
              orderCandidates.push_back( Order(source, destination, shipsAvailableStatic));
            }
            orderCandidates.push_back(Order(source, destination, shipsAvailableCompetitive));
            if (shipsRequired < shipsAvailableCompetitive) {
              orderCandidates.push_back(Order(source, destination, shipsRequired));
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
        && (predictions[p][lookahead].owner()->isNeutral() 
            && competitivePredictions[p][lookahead].owner()->isMe() 
            && (me->shipsCount() 
                + me->growthRate() * distance(enemyPlanets,temp) 
                - predictions[p][lookahead].shipsCount() 
                >= enemy->shipsCount()) 
            && (me->growthRate() <= enemy->growthRate() 
                || myPredictedGrowth <= enemyPredictedGrowth))) {
      int timeToPayoff = predictions[p][lookahead].shipsCount() / p->growthRate();
      if (timeToPayoff < fastestPayoff) {
        fastestPayoff = timeToPayoff;
        fastestPayoffPlanet = p;
      }
    }
  }
  if (fastestPayoff < maxDistanceBetweenPlanets && fastestPayoffPlanet) {
    fastestPayoffPlanet->frontierStatus = true;
    cerr << "Set expansion target: " << fastestPayoffPlanet->planetID() << endl;
    for(Planets::const_iterator pit = planets.begin(); pit != planets.end(); ++pit) {
      Planet* p = *pit;
      if (p != fastestPayoffPlanet &&  protects(fastestPayoffPlanet, p)) {
        p->frontierStatus = false;
      }
    }
  }
  

  for(Planets::const_iterator pit = planets.begin(); pit != planets.end(); ++pit) {
    Planet* p = *pit;
    if ((p->owner()->isEnemy() || predictions[p][lookahead].owner()->isEnemy()) && competitivePredictions[p][lookahead].owner()->isMe()) {
      p->frontierStatus = true;
      for(Planets::const_iterator pit2 = planets.begin(); pit2 != planets.end(); ++pit2) {
        Planet* p2 = *pit;
        if (p2->planetID() != p->planetID() &&  protects(p, p2) && p2->owner()->isMe()) {
          p2->frontierStatus = false;
        }
      }
    }
  }

  if(fastestPayoffPlanet){
    cerr << fastestPayoffPlanet->frontierStatus << endl;
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

vector<Fleet> MyBot::computeMaxOutgoingFleets(Planet* pl){
  vector<Fleet> outFleets;
  Planet p = *pl;
  if(!p.owner()->isNeutral()){
    Fleet f(p.owner(), p.shipsCount());
    outFleets.push_back(f);
    p.shipsCount_m = 0;
  } else {
    outFleets.push_back(Fleet(p.owner(), 0));
  }
  for(int i(0); i != lookahead+1; ++i){
    p = p.getPredictions(1,i)[1];
    if(!p.owner()->isNeutral()){
      outFleets.push_back(Fleet(p.owner(), p.shipsCount()));
      p.shipsCount_m = 0;
    } else {
      outFleets.push_back(Fleet(p.owner(), 0));
    }
  }
  return outFleets;
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

Planet* MyBot::isProtectedFromBy(Planet* pl, Planet* from) const{
  Planets closest = pl->closestPlanets();
  int dist = from->distance(pl);
  for(Planets::const_iterator pit = closest.begin(); pit != closest.end(); ++pit) {
    Planet* p = *pit;
    if (p->owner() == pl->owner() && protects(p, pl, from)) {
      return p;
    }
    if (p->distance(pl) > dist) {
      return pl;
    }
  }
  return pl;
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
          cerr << "Planet " << pl->planetID() << " has been set as Frontier by isFrontier()" << endl;
          return true;
        }
        if (p2->owner()->isMe()  && p2->distance(pl) < pl->distance(p)) {
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

Planet* MyBot::nearestFriendlyPlanet(Planet* pl) const{
  Planets closest = pl->closestPlanets();
  for(Planets::iterator pit = closest.begin(); pit != closest.end(); ++pit) {
    Planet* p = *pit;
    if (p->owner()->isMe())
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
    for (int i(0); i != lookahead+1; ++i) {
      Fleet f = maxOutgoingFleets[p][i];
      if(!f.owner()->isNeutral()){
        Fleet f2(f.owner(), p, pl, f.shipsCount(), dist+i, dist+i);
        fs.push_back(f2);
      }
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
    for (int i(0); i != lookahead+1; ++i) {
      Fleet f = maxOutgoingFleets[p][i];
      if(f.owner()->isEnemy()){
        Fleet f2(f.owner(), p, pl, f.shipsCount(), dist+i, dist+i);
        fs.push_back(f2);
      }
    }
  }
  return fs;
}
 
//Given predictions of a planets future, compute how long I will hold this planet starting at time t. 
//This is used to check if conquering a neutral planet pays off in the worst case.
int MyBot::willHoldFor(const vector<Planet>& predictions,int t) const{
  if(t >= predictions.size()){
    return 0;
  }
  for ( vector<Planet>::const_iterator pFuture = predictions.begin()+t; pFuture != predictions.end(); ++pFuture) {
    if (pFuture->owner()->isEnemy()) return pFuture - (predictions.begin()+t) -1;
  }
  return turnsRemaining;
}
    

//A dynamic programming solution to the knapsack01-problem.
//This is used in the first turn to allocate ships in a way that maximizes the production rate.
Planets MyBot::knapsack01(const Planets& candidates, int maxWeight) const{
  vector<int> weights;
  vector<int> values;

  for (Planets::const_iterator pit = candidates.begin(); pit != candidates.end(); ++pit) {
    Planet* p = *pit;
    weights.push_back(p->shipsCount() + 1);
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
      markedPlanets.push_back(candidates[i-1]);
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
  int available = predictions[0].shipsCount();
  int i(0);
  for(std::vector<Planet>::const_iterator pit = predictions.begin(); pit != predictions.end(); ++pit) {
    int sc = pit->shipsCount();
    //cerr << "sc: " << sc << endl;
    if (pit->owner()->isMe()) {
      available = min(available,sc);
    } else {
      available = min(available, -sc);
      //cerr << available << endl;
      break;
    }
    //cerr << available << endl;
    i++;
    if (i>t)break;
  }
  return available;
}
      

Planet* MyBot::coveredBy(Planet* pl, Planet* from) const{
  Planets closest = from->closestPlanets();
  for(Planets::const_iterator pit = closest.begin(); pit != closest.end(); ++pit){
    Planet* p = *pit;
    if (p == pl){
      return pl;
    }
    if(from->distance(p)+p->distance(pl) == from->distance(pl)){
      return p;
    }
  }
  return pl;
}
  

int MyBot::potential(Planet* pl) const{
  int pot(0);
  Planets closest = pl->closestPlanets();
  for(Planets::const_iterator pit = closest.begin(); pit != closest.end(); ++pit){
    Planet* p = *pit;
    if(pl->distance(p) > maxDistanceBetweenPlanets){
      break;
    }
    if(!p->owner()->isMe()){
      pot += p->growthRate();
    }
  }
  return pot;
}


Planets MyBot::cluster(Planet* pl) const{
  Planets clusterPlanets;
  clusterPlanets.push_back(pl);
  Planet* p = nearestFrontierPlanet(pl);
  Planets closest = p->closestPlanets();
  for(Planets::iterator pit = closest.begin(); pit != closest.end(); ++pit){
    Planet* p2 = *pit;
    if (p2->frontierStatus){
      return clusterPlanets;
    }
    if (p2->owner()->isMe()){
      clusterPlanets.push_back(p2);
    }
  }
  return clusterPlanets;
}



bool MyBot::willHoldAtSomePoint(const vector<Planet>& preds) const{
  for(vector<Planet>::const_iterator p = preds.begin(); p != preds.end(); ++p){
    if(p->owner()->isMe()){
      return true;
    }
  }
  return false;
}

void MyBot::issueOrder(Order o, string reason){
  if(logging){
    cerr << "Sent " << o.shipsCount << " from " << o.sourcePlanet->planetID() << " to " << o.destinationPlanet->planetID() << " because of " << reason << endl;
  }
  game->issueOrder(o);
}
