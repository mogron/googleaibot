#include "order.h"

#include <sstream>
#include "player.h"
#include "planet.h"

#include "MyBot.h"

Order::Order(Planet* sourcePlanet, Planet* destinationPlanet, uint shipsCount) :
    sourcePlanet(sourcePlanet),
    destinationPlanet(destinationPlanet),
    shipsCount(shipsCount)
{
}

bool Order::isValid() const
{
    return ((sourcePlanet->planetID() != destinationPlanet->planetID()) // Source can not be equal to destination
         && (sourcePlanet->owner()->isMe())                             // I must own the source planet
         && (shipsCount > 0)                                            // I must send more then 0 ships
         && (sourcePlanet->shipsCount() >= shipsCount)                  // The source planet must have enought ships
         );
}
