#ifndef COMPARATOR_H
#define COMPARATOR_H

#include "defines.h"

#include "planet.h"
#include "fleet.h"

namespace Comparator
{
    class Distance {
    public:
        Distance(const Point& a = Point(0, 0)) : a(a) {}

        void setInitialPoint(const Point& newA) { a = newA; }

        bool operator () (const Planet* p1, const Planet* p2) const;
        bool operator () (const Planet& p1, const Planet& p2) const;
        bool operator () (const Point& p1, const Point& p2) const;

    private:
        Point a;
    };

    class ShipsCount {
    public:
        ShipsCount() {}

        bool operator () (const Planet* p1, const Planet* p2) const;
        bool operator () (const Planet& p1, const Planet& p2) const;
    };

    class GrowthRate {
    public:
        GrowthRate() {}

        bool operator () (const Planet* p1, const Planet* p2) const;
        bool operator () (const Planet& p1, const Planet& p2) const;
    };

    class TurnsRemaining {
    public:
        bool operator () (const Fleet* f1, const Fleet* f2) const;
        bool operator () (const Fleet& f1, const Fleet& f2) const;
    };

    template <typename Container, typename Comparator>
    Container sortCopy(const Container& container, Comparator comp) {
        Container sorted(container);
        std::sort(sorted.begin(), sorted.end(), comp);
        return sorted;
    }

    template <typename Container, typename Comparator>
    void sort(Container& container, Comparator comp) {
        std::sort(container.begin(), container.end(), comp);
    }
};

#endif // COMPARATOR_H
