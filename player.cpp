#include "defines.h"
#include "planet.h"
#include "fleet.h"
#include "player.h"

Player::Player(uint playerID) :
    playerID_m(playerID)
{
}

Planets Player::planets() const
{
    return planets_m;
}

Fleets Player::fleets() const
{
    return fleets_m;
}

void Player::addFleet(Fleet* fleet)
{
    fleets_m.push_back(fleet);
}

void Player::addPlanet(Planet* planet)
{
    planets_m.push_back(planet);
}

void Player::clearFleets()
{
    fleets_m.clear();
}

void Player::clearPlanets()
{
    planets_m.clear();
}

uint Player::playerID() const
{
    return playerID_m;
}

bool Player::isEnemy(int enemyID) const
{
    if (enemyID < 2) {
        return playerID_m > 1;
    }
    else {
        return playerID_m == enemyID;
    }
}

bool Player::isNeutral() const
{
    return playerID_m == 0;
}

bool Player::isMe() const
{
    return playerID_m == 1;
}

bool Player::operator == (const Player& player) const
{
    return playerID_m == player.playerID();
}

bool Player::operator != (const Player& player) const
{
    return playerID_m != player.playerID();
}

int Player::shipsCount()
{
  int sc = 0;
  for (Planets::iterator pit = planets_m.begin(); pit != planets_m.end(); ++pit){
    Planet* p = *pit;
    sc += p->shipsCount();
  }
  for (Fleets::iterator fit = fleets_m.begin(); fit != fleets_m.end(); ++fit){
    Fleet* f = *fit;
    sc += f->shipsCount();
  }
  return sc;
}

int Player::growthRate(){
  int gr = 0;
  for (Planets::iterator pit = planets_m.begin(); pit != planets_m.end(); ++pit){
    Planet* p = *pit;
    gr += p->growthRate();
  }
  return gr;
}
