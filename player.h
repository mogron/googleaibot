#ifndef PLAYER_H
#define PLAYER_H

#include "defines.h"

class Player
{
    friend class Game;
public:
    Player(uint playerID);

    Planets planets() const;
    Fleets fleets() const;

    bool isEnemy(int enemyID = -1) const;
    bool isNeutral() const;
    bool isMe() const;

    bool operator == (const Player& player) const;
    bool operator != (const Player& player) const;

    int shipsCount();
    int shipsOnPlanets();
    int growthRate();
private:
    uint playerID() const;
    void addPlanet(Planet* planet);
    void addFleet(Fleet* fleet);
    void clearPlanets();
    void clearFleets();

    uint playerID_m;

    Planets planets_m;
    Fleets fleets_m;
};

#endif // PLAYER_H
