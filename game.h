#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <string>

#include "defines.h"

class Game {
public:
    Game();
    ~Game();

    void initializeState(const std::string& state);
    void updateState(const std::string& state);

    uint planetsCount() const;
    uint fleetsCount() const;

    uint turn() const;

    const Player* playerByID(uint playerID) const;
    Player* playerByID(uint playerID);

    const Planet* planetByID(uint planetID) const;
    Planet* planetByID(uint planetID);


    Planets const& planets() const;
    Planets myPlanets() const;
    Planets neutralPlanets() const;
    Planets enemyPlanets() const;
    Planets notMyPlanets() const;

    Fleets const& fleets() const;
    Fleets myFleets() const;
    Fleets enemyFleets() const;

    void issueOrder(const Order& order);

    void finishTurn() const;

private:
    void updateState(Order order);
    void deleteFleets();
    void deletePlanets();
    void deletePlayers();

    void clearPlayersFleets();
    void clearPlanetsFleets();
    void clearPlayersPlanets();



    uint turn_m;
    uint planetsCount_m;
    uint playersCount_m;

    // Store all the planets and fleets.
    Planets planets_m;
    Fleets  fleets_m;
    std::vector<Player*> players_m;
};

#endif // GAMESTATE_H
