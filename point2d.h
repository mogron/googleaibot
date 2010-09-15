#ifndef POINT2D_H
#define POINT2D_H

#include "defines.h"

template<typename Type>
class Point2D
{
public:
    Point2D(Type x, Type y) : x_m(x), y_m(y) {}

    Type x() { return x_m; }
    Type y() { return y_m; }

    static uint distanceBetween(Point2D<Type> source, Point2D<Type> destination)
    {
        Type dx = source.x() - destination.x();
        Type dy = source.y() - destination.y();
        return (unsigned int)ceil(sqrt(dx * dx + dy * dy));
    }

    static uint relativeDistanceBetween(Point2D<Type> source, Point2D<Type> destination)
    {
        Type dx = source.x() - destination.x();
        Type dy = source.y() - destination.y();
        return (unsigned int)ceil(dx * dx + dy * dy);
    }

    uint distanceFrom(Point2D<Type> destination) const
    {
        return distanceBetween(Point2D<Type>(x_m, y_m), destination);
    }

private:
    Type x_m;
    Type y_m;
};

#endif // POINT2D_H
