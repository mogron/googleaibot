#ifndef MINIBOT_H
#define MINIBOT_H

#include "abstractbot.h"
#include <vector>
#include <map>

class MyBot : public AbstractBot
{
public:
    MyBot(Game* game);
    int max_distance_between_planets;
    void initialize();

    Planet* myStartingPlanet;
    Planet* enemyStartingPlanet;
    Planets planets;
    int turnsRemaining;

    void executeTurn();
    static void log(char *format, ...);

 private:
    int myPredictedGrowthRate(int t);
    int enemyPredictedGrowthRate(int t);
    std::map<Planet*, std::vector<Planet > > predictions;
    std::map<Planet*, std::vector<Planet > > competitivePredictions;
    std::map<Planet*, std::vector<Planet > > worstCasePredictions;
    std::map<Planet*, bool> frontierStatus;
    int supplyMove(Planet* pl, Planet* goal);
    Planets knapsack01(Planets planets, int maxWeight);
    int value(std::vector<Planet> predictions);
    bool protects(Planet* protector, Planet* protectee, Planet* from);
    bool protects(Planet* protector, Planet* protectee);
    bool isProtectedFrom(Planet* pl, Planet* from);
    bool isProtected(Planet* pl);
    Planet* nearestFrontierPlanet(Planet* pl);
    int distanceToFrontier(Planet* pl);
    Planet* nextPlanetCloserToFrontier(Planet* pl);
    bool isFrontier(Planet* pl);
    std::vector<Fleet> competitiveFleets(Planet* pl);
    std::vector<Fleet> worstCaseFleets(Planet* pl);
    int willHoldFor(const vector<Planet>& predictions, int t);
};

#endif // MINIBOT_H
