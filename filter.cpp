#include "filter.h"

// Only my PLANETS

bool Filter::OnlyMyPlanets::operator () (const Planet* planet) const
{
    return !planet->owner()->isMe();
}

bool Filter::OnlyMyPlanets::operator () (const Planet& planet) const
{
    return operator ()(&planet);
}

// Only enemy PLANETS

bool Filter::OnlyEnemyPlanets::operator () (const Planet* planet) const
{
    return !planet->owner()->isEnemy(enemyID);
}

bool Filter::OnlyEnemyPlanets::operator () (const Planet& planet) const
{
    return operator ()(&planet);
}

// Only neutral PLANETS

bool Filter::OnlyNeutralPlanets::operator () (const Planet* planet) const
{
    return !planet->owner()->isNeutral();
}

bool Filter::OnlyNeutralPlanets::operator () (const Planet& planet) const
{
    return operator ()(&planet);
}

// Only not my PLANETS

bool Filter::OnlyNotMyPlanets::operator () (const Planet* planet) const
{
    return planet->owner()->isMe();
}

bool Filter::OnlyNotMyPlanets::operator () (const Planet& planet) const
{
    return operator ()(&planet);
}

// Only my FLEETS

bool Filter::OnlyMyFleets::operator () (const Fleet* fleet) const
{
    return !fleet->owner()->isMe();
}

bool Filter::OnlyMyFleets::operator () (const Fleet& fleet) const
{
    return operator ()(&fleet);
}

// Only enemy FLEETS

bool Filter::OnlyEnemyFleets::operator () (const Fleet* fleet) const
{
    return !fleet->owner()->isEnemy(enemyID);
}

bool Filter::OnlyEnemyFleets::operator () (const Fleet& fleet) const
{
    return operator ()(&fleet);
}

