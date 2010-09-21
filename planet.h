#ifndef PLANET_H
#define PLANET_H

#include "defines.h"
#include "point2d.h"

class Planet {
    friend class Game;
public:
    // Initializes a planet.
    Planet(uint planetID, uint shipsCount, uint growthRate, Point coordinate, const Player* owner);

    void updateFuture(int t);

    std::vector<Planet> getPredictions();
    uint planetID() const;
    uint shipsCount() const;
    uint growthRate() const;
    const Player* owner() const;


    Point coordinate() const;
    Planets closestPlanets();

    bool frontier_m;
    bool isFrontier();
    void updateFrontierStatus();
    Planet* nearestFrontierPlanet();
    std::vector<Planet> predictions_m;
    int shipsAvailable();
    int distance(Planet* p);
    int timeToPayoff();
private:
    void update(const Player* owner, uint shipsCount);
    void setOtherPlanets(const Planets& planets);
    void addIncomingFleet(Fleet* fleet);
    void addLeavingFleet(Fleet* fleet);
    void clearFleets();

    uint planetID_m;
    uint shipsCount_m;
    uint growthRate_m;
    Point coordinate_m;
    const Player* owner_m;

    // Pointers to all other planets. Sorted by distance in ascending order.
    Planets closestPlanets_m;
    Fleets incomingFleets_m;
    Fleets leavingFleets_m;
};


#endif // PLANET_H
