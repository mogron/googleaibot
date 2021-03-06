#ifndef PLANET_H
#define PLANET_H

#include "defines.h"
#include "point2d.h"
#include "stlastar.h"

#include <list>

class Planet {
    friend class Game;
public:
    // Initializes a planet.
    Planet(int planetID, int shipsCount, int growthRate, Point coordinate, const Player* owner);
    Planet();

    std::vector<Planet> getPredictions(int t, std::list<Fleet> fs, int start = 0);
    std::vector<Planet> getPredictions(int t, int start = 0);
    int planetID() const;
    int shipsCount() const;
    int growthRate() const;
    const Player* owner() const;


    Point coordinate() const;
    Planets closestPlanets() const;


    int distance(const Planet* p);
    int distance(const Planets& ps);
    int timeToPayoff() const;

    bool onMySide;
    


    //A* specific stuff:
    float GoalDistanceEstimate( Planet &nodeGoal );
    bool IsGoal( Planet &nodeGoal );
    bool GetSuccessors( AStarSearch<Planet> *astarsearch, Planet *parent_node );
    float GetCost( Planet &successor );
    bool IsSameState( Planet &rhs );
    
    void PrintNodeInfo(); 




    bool frontierStatus;
    bool predictedMine;
    bool predictedEnemy;

    int shipsCount_m;
    Fleets incomingFleets_m;
private:
    void update(const Player* owner, int shipsCount);
    void setOtherPlanets(const Planets& planets);
    void addIncomingFleet(Fleet* fleet);
    void addLeavingFleet(Fleet* fleet);
    void clearFleets();

    int planetID_m;
    int growthRate_m;
    Point coordinate_m;
    const Player* owner_m;

    // Pointers to all other planets. Sorted by distance in ascending order.
    Planets closestPlanets_m;
    Fleets leavingFleets_m;

};


#endif // PLANET_H
