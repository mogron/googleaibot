#ifndef DEFINES_H
#define DEFINES_H

#include <vector>
#include <algorithm>
#include <cmath>

class Game;
class Planet;
class Fleet;
class Order;
class Player;
template<typename Type>
class Point2D;

typedef unsigned int uint;
typedef Point2D<double> Point;

typedef std::vector<Planet*> Planets;
typedef std::vector<Fleet*> Fleets;
typedef std::vector<Order> Orders;
typedef std::vector<Player*> Players;

#endif // DEFINES_H
