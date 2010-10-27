#ifndef MY_BOT_H
#define MY_BOT_H

#include "abstractbot.h"
#include "defines.h"
#include "planet.h"
#include <vector>
#include <map>

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


    //action finding:
    void firstTurn();
    Planets knapsack01(const Planets& planets, int maxWeight) const;
    bool chooseAction();
    void addOrderCandidates(Planet* source, Orders& orderCandidates);
    int value(Order* oit);
    int value(const std::vector<Planet>& predictions) const;
    void supply();
    int supplyMove(Planet* pl, Planet* goal);
    void setExpansionTargets();

    
    //predictions:
    std::map<Planet*, std::vector<Planet > > predictions;
    std::map<Planet*, std::vector<Planet > > competitivePredictions;
    std::map<Planet*, std::vector<Planet > > worstCasePredictions;
    std::vector<Fleet> competitiveFleets(Planet* pl);
    std::vector<Fleet> worstCaseFleets(Planet* pl);
    int willHoldFor(const vector<Planet>& predictions, int t) const;
    int shipsAvailable(const vector<Planet>& predictions, int t) const;
    int myPredictedGrowth;
    int enemyPredictedGrowth;

    //protects and frontier stuff:
    //TODO: separate from MyBot class
    bool protects(Planet* protector, Planet* protectee, Planet* from) const;
    bool protects(Planet* protector, Planet* protectee) const;
    bool isProtectedFrom(Planet* pl, Planet* from) const;
    bool isProtected(Planet* pl) const;
    Planet* nearestFrontierPlanet(Planet* pl) const;
    int distanceToFrontier(Planet* pl) const;
    Planet* nextPlanetCloserToFrontier(Planet* pl) const;
    bool isFrontier(Planet* pl) const;
    int distance(const Planets& ps1, const Planets& ps2) const;
};

#endif // MY_BOT_H
