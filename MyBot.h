#ifndef MY_BOT_H
#define MY_BOT_H

#include "abstractbot.h"
#include "defines.h"
#include "planet.h"
#include "knapsackTarget.h"
#include <vector>
#include <list>
#include <map>
#include <sys/time.h>

class MyBot : public AbstractBot
{
public:
    MyBot(Game* game);

    void executeTurn();

 private:
    //setup functions and state information:
    void initialize();
    void preprocessing();
    int myPredictedGrowthRate(int t);
    int enemyPredictedGrowthRate(int t);
    Planets myPlanets;
    Planets notMyPlanets;
    Planets enemyPlanets;
    Planets neutralPlanets;
    Planet* myStartingPlanet;
    Planet* enemyStartingPlanet;
    Planets planets;
    int maxDistanceBetweenPlanets;
    int turnsRemaining;
    Player* me;
    Player* enemy;
    int lookahead;
    bool logging;

    void logOrders(const Orders& os);

    //action finding:
    void openingTurn();
    vector<KnapsackTarget> knapsack01(const vector<KnapsackTarget>& planets, int maxWeight) const;
    bool chooseAction();
    void addOrderCandidates(Planet* source1, Planet* source2, vector<Orders>& orderCandidates);
    int value(const Orders& os, bool worstcase = false);
    int value(const std::vector<Planet>& predictions) const;
    void supply();
    int supplyMove(Planet* pl, Planet* goal);
    void setExpansionTargets();
    void panic();
    int potential(Planet* pl);
    Orders optimizeOrders(const Orders& o);
    list<Fleet> ordersToFleets(const Orders& os);


    //predictions:
    void updatePredictions();
    std::map<Planet*, std::vector<Planet > > predictions;
    std::map<Planet*, std::vector<Planet > > competitivePredictions;
    std::map<Planet*, std::vector<Planet > > worstCasePredictions;
    std::map<Planet*, std::list<Fleet > > maxOutgoingFleets;
    list<Fleet> computeMaxOutgoingFleets(Planet* pl);
    std::list<Fleet> competitiveFleets(Planet* pl);
    std::list<Fleet> worstCaseFleets(Planet* pl);
    int willHoldFor(const vector<Planet>& predictions, int t) const;
    int shipsAvailable(const vector<Planet>& predictions, int t) const;
    int myPredictedGrowth;
    int enemyPredictedGrowth;

    //protects and frontier stuff:
    //TODO: separate from MyBot class
    bool protects(Planet* protector, Planet* protectee, Planet* from) const;
    bool protects(Planet* protector, Planet* protectee) const;
    bool isProtectedFrom(Planet* pl, Planet* from) const;
    Planet* isProtectedFromBy(Planet* pl, Planet* from) const;
    bool isProtected(Planet* pl) const;
    Planet* nearestFrontierPlanet(Planet* pl) const;
    int distanceToFrontier(Planet* pl) const;
    Planet* nextPlanetCloserToFrontier(Planet* pl) const;
    bool isFrontier(Planet* pl) const;
    int distance(const Planets& ps1, const Planets& ps2) const;
    Planet* nearestEnemyPlanet(Planet* pl) const;
    Planet* nearestFriendlyPlanet(Planet* pl) const;
    Planet* coveredBy(Planet* pl, Planet* from) const;
    Planets cluster(Planet* pl) const;
    bool willHoldAtSomePoint(const vector<Planet>& preds) const;

    void issueOrder(Order o, string reason);
    void issueOrders(const Orders& os);
    
    timeval tim;
    double t1;
    double t2;
    double maxTime;
};

#endif // MY_BOT_H
