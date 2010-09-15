#ifndef PARSER_H
#define PARSER_H

#include "fleet.h"
#include "planet.h"
#include "player.h"
#include "order.h"
#include "stringtokenizer.h"

class Parser
{
public:
    static std::string orderToString(const Order& order);
private:
    Parser();
};

#endif // PARSER_H
