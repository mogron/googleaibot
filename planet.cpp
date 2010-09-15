#include "planet.h"

#include "comparator.h"
#include "fleet.h"

Planet::Planet(uint planetID, uint shipsCount, uint growthRate, Point coordinate, const Player* owner) :
    planetID_m(planetID),
    shipsCount_m(shipsCount),
    growthRate_m(growthRate),
    coordinate_m(coordinate),
    owner_m(owner),
    closestPlanets_m(0)
{
}

void Planet::update(const Player* owner, uint shipsCount)
{
    owner_m = owner;
    shipsCount_m = shipsCount;
}

uint Planet::planetID() const
{
    return planetID_m;
}

const Player* Planet::owner() const
{
    return owner_m;
}

uint Planet::shipsCount() const
{
    return shipsCount_m;
}

uint Planet::growthRate() const
{
    return growthRate_m;
}

Point Planet::coordinate() const
{
    return coordinate_m;
}

Planets Planet::closestPlanets()
{
    return closestPlanets_m;
}

void Planet::setOtherPlanets(const Planets& planets)
{
    closestPlanets_m = planets;
    closestPlanets_m.erase(std::find(closestPlanets_m.begin(), closestPlanets_m.end(), this));

    Comparator::Distance compareDistance(coordinate_m);
    Comparator::sort(closestPlanets_m, compareDistance);
}

void Planet::addIncomingFleet(Fleet* fleet)
{
    incomingFleets_m.push_back(fleet);
}

void Planet::addLeavingFleet(Fleet* fleet)
{
    leavingFleets_m.push_back(fleet);
}

void Planet::clearFleets()
{
    leavingFleets_m.clear();
    incomingFleets_m.clear();
}
