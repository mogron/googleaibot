#include "parser.h"

#include <sstream>

Parser::Parser()
{
}

std::string Parser::orderToString(const Order& order)
{
    std::stringstream stream;
    stream << order.sourcePlanet->planetID() << " " << order.destinationPlanet->planetID() << " " << order.shipsCount << std::endl;
    return stream.str();
}
