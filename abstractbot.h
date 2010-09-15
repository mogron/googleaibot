#ifndef ABSTRACTBOT_H
#define ABSTRACTBOT_H

#include "defines.h"
#include "game.h"
#include "filter.h"
#include "comparator.h"

using Comparator::sort;
using Comparator::sortCopy;
using Filter::filter;
using Filter::filterCopy;

class AbstractBot
{
public:
    AbstractBot(Game* game);

    // This is your main function in your bot which is called each turn
    virtual void executeTurn() = 0;

    // We have these comparators and predicates as members so that we can use them without making a new time we wish to compare or sort something
    // For onlyEnemyPlanets you can set specific enemy ID and for compareByDistance you have to set the initial point before comparison
    Filter::OnlyEnemyFleets onlyEnemyFleets;
    Filter::OnlyMyFleets onlyMyFleets;
    Filter::OnlyEnemyPlanets onlyEnemyPlanets;
    Filter::OnlyMyPlanets onlyMyPlanets;
    Filter::OnlyNeutralPlanets onlyNeutralPlanets;
    Filter::OnlyNotMyPlanets onlyNotMyPlanets;

    Comparator::Distance compareByDistane;
    Comparator::GrowthRate compareByGrowthRate;
    Comparator::ShipsCount compareByShipsCount;

    // Const pointer to game... no reason to make setter getter
    Game* const game;
};

#endif // ABSTRACTBOT_H
