#include "fleet.h"

#include "order.h"
#include "planet.h"
#include "point2d.h"

Fleet::Fleet(const Player* owner, const Planet* sourcePlanet, const Planet* destinationPlanet, uint shipsCount, uint tripLength, uint turnsRemaining) :
    owner_m(owner),
    sourcePlanet_m(sourcePlanet),
    destinationPlanet_m(destinationPlanet),
    shipsCount_m(shipsCount),
    tripLenght_m(tripLength),
    turnsRemaining_m(turnsRemaining)
{
}

Fleet::Fleet(const Order& order) :
    owner_m(order.sourcePlanet->owner()),
    sourcePlanet_m(order.sourcePlanet),
    destinationPlanet_m(order.destinationPlanet),
    shipsCount_m(order.shipsCount),
    tripLenght_m(Point::distanceBetween(sourcePlanet_m->coordinate(), destinationPlanet_m->coordinate())),
    turnsRemaining_m(tripLenght_m)
{
}

const Player* Fleet::owner() const
{
    return owner_m;
}

const Planet* Fleet::sourcePlanet() const
{
    return sourcePlanet_m;
}

const Planet* Fleet::destinationPlanet() const
{
    return destinationPlanet_m;
}

uint Fleet::shipsCount() const
{
    return shipsCount_m;
}

uint Fleet::tripLength() const
{
    return tripLenght_m;
}

uint Fleet::turnsRemaining() const
{
    return turnsRemaining_m;
}
