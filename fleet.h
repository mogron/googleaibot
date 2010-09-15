#ifndef FLEET_H
#define FLEET_H

#include "defines.h"

class Fleet {
    friend class Game;
public:
    Fleet(const Player* owner, const Planet* sourcePlanet, const Planet* destinationPlanet, uint shipsCount, uint tripLength, uint turnsRemaining);
    Fleet(const Order& order);

    const Player* owner() const;
    uint shipsCount() const;

    const Planet* sourcePlanet() const;
    const Planet* destinationPlanet() const;

    uint tripLength() const;
    uint turnsRemaining() const;

private:
    const Player* owner_m;
    const Planet* sourcePlanet_m;
    const Planet* destinationPlanet_m;
    uint shipsCount_m;

    uint tripLenght_m;
    uint turnsRemaining_m;
};

#endif // FLEET_H
