#include "comparator.h"

// Distance

bool Comparator::Distance::operator () (const Point& p1, const Point& p2) const
{
    return Point::relativeDistanceBetween(a, p1) < Point::relativeDistanceBetween(a, p2);
}

bool Comparator::Distance::operator () (const Planet* p1, const Planet* p2) const
{
    return operator ()(p1->coordinate(), p2->coordinate());
}

bool Comparator::Distance::operator () (const Planet& p1, const Planet& p2) const
{
    return operator ()(p1.coordinate(), p2.coordinate());
}

// Ships Count

bool Comparator::ShipsCount::operator () (const Planet* p1, const Planet* p2) const
{
    return p1->shipsCount() < p2->shipsCount();
}

bool Comparator::ShipsCount::operator () (const Planet& p1, const Planet& p2) const
{
    return operator ()(&p1, &p2);
}

// Growth Rate

bool Comparator::GrowthRate::operator () (const Planet* p1, const Planet* p2) const
{
    return p1->growthRate() < p2->growthRate();
}

bool Comparator::GrowthRate::operator () (const Planet& p1, const Planet& p2) const
{
    return operator ()(&p1, &p2);
}

// Turns Remaining

bool Comparator::TurnsRemaining::operator () (const Fleet* f1, const Fleet* f2) const
{
    return f1->turnsRemaining() < f2->turnsRemaining();
}

bool Comparator::TurnsRemaining::operator () (const Fleet& f1, const Fleet& f2) const
{
    return operator ()(&f1, &f2);
}
