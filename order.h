#ifndef ORDER_H
#define ORDER_H

#include <string>
#include "defines.h"

class Order {
public:
    Order(Planet* sourcePlanet, Planet* destinationPlanet, uint numberOfShips);

    Planet* sourcePlanet;
    Planet* destinationPlanet;

    uint shipsCount;

    bool isValid() const;
};

#endif // ORDER_H
