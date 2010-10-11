#include "planet.h"

#include <map>
#include <algorithm>
#include <vector>
#include <iostream>
#include "comparator.h"
#include "fleet.h"
#include "player.h"

using std::min;
using std::vector;
using std::cerr;
using std::endl;

Planet::Planet(uint planetID, uint shipsCount, uint growthRate, Point coordinate, const Player* owner) :
    planetID_m(planetID),
    shipsCount_m(shipsCount),
    growthRate_m(growthRate),
    coordinate_m(coordinate),
    owner_m(owner),
    closestPlanets_m(0)
{
}

void Planet::update(const Player* owner, uint shipsCount)
{
    owner_m = owner;
    shipsCount_m = shipsCount;
}

uint Planet::planetID() const
{
    return planetID_m;
}

const Player* Planet::owner() const
{
    return owner_m;
}

uint Planet::shipsCount() const
{
    return shipsCount_m;
}

uint Planet::growthRate() const
{
    return growthRate_m;
}

Point Planet::coordinate() const
{
    return coordinate_m;
}

Planets Planet::closestPlanets()
{
    return closestPlanets_m;
}

void Planet::setOtherPlanets(const Planets& planets)
{
    closestPlanets_m = planets;
    closestPlanets_m.erase(std::find(closestPlanets_m.begin(), closestPlanets_m.end(), this));

    Comparator::Distance compareDistance(coordinate_m);
    Comparator::sort(closestPlanets_m, compareDistance);
}

void Planet::addIncomingFleet(Fleet* fleet)
{
    incomingFleets_m.push_back(fleet);
}

void Planet::addLeavingFleet(Fleet* fleet)
{
    leavingFleets_m.push_back(fleet);
}

void Planet::clearFleets()
{
    leavingFleets_m.clear();
    incomingFleets_m.clear();
}

int Planet::distance(const Planet* p)
{
  return Point::distanceBetween(coordinate(),p->coordinate());
}


vector<Planet> Planet::getPredictions(int t)
{
  vector<Fleet> fs;
  return getPredictions(t, fs);
}

vector <Planet> Planet::getPredictions(int t, vector<Fleet> fs)
{
  vector<Planet> predictions;
  Planet p(*this);
  for(int i(0);i!=t+1;i++){
    if(i!=0 && !p.owner_m->isNeutral()){
      p.shipsCount_m += p.growthRate_m;
    }
    std::map<const Player*,int> participants;
    participants[p.owner_m] = p.shipsCount_m;
    for (Fleets::iterator it = incomingFleets_m.begin(); it != incomingFleets_m.end(); ++it ) {
      Fleet* f = *it;
      if (f->turnsRemaining() == i ) {
        participants[f->owner()] += f->shipsCount();
      }
    }

    for (vector<Fleet>::iterator f = fs.begin(); f != fs.end(); ++f ) {
      if (f->destinationPlanet() == this && f->turnsRemaining() == i ) {
        participants[f->owner()] += f->shipsCount();
      }
      if (f->sourcePlanet() == this && f->turnsRemaining()-i == this->distance(f->destinationPlanet())){
        participants[f->owner()] -= f->shipsCount();
      }
     }
    
    

    Fleet winner(0, 0);
    Fleet second(0, 0);
    for (std::map<const Player*,int>::iterator f = participants.begin(); f != participants.end(); ++f) {
      if (f->second > second.shipsCount()) {
        if(f->second > winner.shipsCount()) {
          second = winner;
          winner = Fleet(f->first, f->second);
        } else {
          second = Fleet(f->first, f->second);
        }
      }
    }
 
    if (winner.shipsCount() > second.shipsCount()) {
      p.shipsCount_m = winner.shipsCount() - second.shipsCount();
      p.owner_m = winner.owner();
    } else {
      p.shipsCount_m = 0;
    }
    predictions.push_back(p);
  }
  return predictions;
}

//if the planet is conquered NOW, how long will it take to pay back lost ships + 20 ships? Meant to be used on future versions of the planet.
int Planet::timeToPayoff()
{
  if (growthRate() == 0){
    return 1000;
  }
  int debt = 60;
  if(!owner_m->isEnemy()){
    debt += shipsCount_m;
  }
  int payoff_time = debt / growthRate();
  return payoff_time;
}



int Planet::shipsAvailable(vector<Planet> predictions, int t)
{
  int available = 100000;
  int i(0);
  for(std::vector<Planet>::const_iterator pit = predictions.begin(); pit != predictions.end(); ++pit){
    int sc = pit->shipsCount();
    if(pit->owner()->isMe()){
      available = min(available,sc);
    } else {
      available = min(available, -sc);
      break;
    }
    i++;
    if(i>t)break;
  }
  return available;
}


