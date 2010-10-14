#ifndef PLANET_H
#define PLANET_H

#include "defines.h"
#include "point2d.h"
#include "stlastar.h"

class Planet {
    friend class Game;
public:
    // Initializes a planet.
    Planet(uint planetID, uint shipsCount, uint growthRate, Point coordinate, const Player* owner);
    Planet();

    std::vector<Planet> getPredictions(int t, std::vector<Fleet> fs);
    std::vector<Planet> getPredictions(int t);
    uint planetID() const;
    uint shipsCount() const;
    uint growthRate() const;
    const Player* owner() const;


    Point coordinate() const;
    Planets closestPlanets();


    int shipsAvailable(std::vector<Planet>, int t);
    int distance(const Planet* p);
    int distance(const Planets& ps);
    int timeToPayoff();


    //A* specific stuff:
    float GoalDistanceEstimate( Planet &nodeGoal );
    bool IsGoal( Planet &nodeGoal );
    bool GetSuccessors( AStarSearch<Planet> *astarsearch, Planet *parent_node );
    float GetCost( Planet &successor );
    bool IsSameState( Planet &rhs );
    
    void PrintNodeInfo(); 

    bool frontierStatus;
    bool predictedMine;


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
