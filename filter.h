#ifndef FILTER_H
#define FILTER_H

#include "defines.h"
#include "planet.h"
#include "fleet.h"
#include "player.h"

namespace Filter
{
    class OnlyMyPlanets
    {
    public:
        bool operator () (const Planet* planet) const;
        bool operator () (const Planet& planet) const;
    };

    class OnlyEnemyPlanets
    {
    public:
        OnlyEnemyPlanets(int enemyID = -1) : enemyID(enemyID) {}

        void setEnemyID(int newID) { enemyID = newID; }

        bool operator () (const Planet* planet) const;
        bool operator () (const Planet& planet) const;

    private:
        int enemyID;
    };

    class OnlyNeutralPlanets
    {
    public:
        bool operator () (const Planet* planet) const;
        bool operator () (const Planet& planet) const;
    };

    class OnlyNotMyPlanets
    {
    public:
        bool operator () (const Planet* planet) const;
        bool operator () (const Planet& planet) const;
    };

    class OnlyMyFleets
    {
    public:
        bool operator () (const Fleet* fleet) const;
        bool operator () (const Fleet& fleet) const;
    };

    class OnlyEnemyFleets
    {
    public:
        OnlyEnemyFleets(int enemyID = -1) : enemyID(enemyID) {}

        void setEnemyID(int newID) { enemyID = newID; }

        bool operator () (const Fleet* fleet) const;
        bool operator () (const Fleet& fleet) const;

    private:
        int enemyID;
    };

    template <typename Container, typename Predicate>
    Container filterCopy(const Container& container, Predicate pred) {
        Container filtered(container);
        filtered.erase(std::remove_if(filtered.begin(), filtered.end(), pred), filtered.end());
        return filtered;
    }

    template <typename Container, typename Predicate>
    void filter(Container& container, Predicate pred) {
        container.erase(std::remove_if(container.begin(), container.end(), pred), container.end());
    }
};

#endif // FILTER_H
