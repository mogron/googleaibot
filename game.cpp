#include "game.h"

#include <iostream>

#include "fleet.h"
#include "planet.h"
#include "player.h"
#include "order.h"
#include "filter.h"
#include "stringtokenizer.h"


Game::Game() :
    turn_m(0),
    planetsCount_m(0),
    playersCount_m(3)
{
    // Add the default number of players
    for (uint i = 0; i < 3; ++i) {
        players_m.push_back(new Player(i));
    }
}

Game::~Game()
{
    deleteFleets();
    deletePlanets();
    deletePlayers();
}

uint Game::turn() const
{
    return turn_m;
}

void Game::deleteFleets()
{
    // Clear fleets from players to not have dangling pointers
    clearPlayersFleets();
    // Clear incoming and leaving fleets from planets to not have dangling pointers
    clearPlanetsFleets();
    uint size = fleets_m.size();
    for (uint i = 0; i < size; ++i) {
        delete fleets_m.at(i);
    }

    fleets_m.clear();
}

void Game::deletePlanets()
{
    // Clear planets from players to not have dangling pointers
    clearPlayersPlanets();
    for (uint i = 0; i < planetsCount_m; ++i){
        delete planets_m.at(i);
    }
    planets_m.clear();
}

void Game::deletePlayers()
{
    for (uint i = 0; i < playersCount_m; ++i) {
        delete players_m.at(i);
    }
    players_m.clear();
}

void Game::clearPlanetsFleets()
{
    for (uint i = 0; i < planetsCount_m; ++i) {
        planets_m.at(i)->clearFleets();
    }
}

void Game::clearPlayersFleets()
{
    for (uint i = 0; i < playersCount_m; ++i) {
        players_m.at(i)->clearFleets();
    }
}

void Game::clearPlayersPlanets()
{
    for (uint i = 0; i < playersCount_m; ++i) {
        players_m.at(i)->clearPlanets();
    }
}

uint Game::planetsCount() const
{
    return planets_m.size();
}

uint Game::fleetsCount() const
{
    return fleets_m.size();
}

const Player* Game::playerByID(uint playerID) const
{
    return playerByID(playerID);
}

Player* Game::playerByID(uint playerID)
{
    Player* player = 0;

    uint size = players_m.size();
    for (uint i = 0; i < size; ++i) {
        if (players_m.at(i)->playerID() == playerID) {
            player = players_m.at(i);
        }
    }

    return player;
}

Planets const& Game::planets() const
{
    return planets_m;
}

Planets Game::myPlanets() const
{
    Planets myPlanets(planets_m);
    Filter::OnlyMyPlanets onlyMyPlanets;
    Filter::filter(myPlanets, onlyMyPlanets);
    return myPlanets;
}

Planets Game::neutralPlanets() const
{
    Planets neutralPlanets(planets_m);
    Filter::OnlyNeutralPlanets onlyNeutralPlanets;
    Filter::filter(neutralPlanets, onlyNeutralPlanets);
    return neutralPlanets;
}

Planets Game::enemyPlanets() const
{
    Planets enemyPlanets(planets_m);
    Filter::OnlyEnemyPlanets onlyEnemyPlanets;
    Filter::filter(enemyPlanets, onlyEnemyPlanets);
    return enemyPlanets;
}

Planets Game::notMyPlanets() const
{
    Planets notMyPlanets(planets_m);
    Filter::OnlyNotMyPlanets onlyNotMyPlanets;
    Filter::filter(notMyPlanets, onlyNotMyPlanets);
    return notMyPlanets;
}

Fleets const& Game::fleets() const
{
    return fleets_m;
}

Fleets Game::myFleets() const
{
    Fleets myFleets(fleets_m);
    Filter::OnlyMyFleets onlyMyFleets;
    Filter::filter(myFleets, onlyMyFleets);
    return myFleets;
}

Fleets Game::enemyFleets() const
{
    Fleets enemyFleets(fleets_m);
    Filter::OnlyEnemyFleets onlyEnemyFleets;
    Filter::filter(enemyFleets, onlyEnemyFleets);
    return enemyFleets;
}

void Game::issueOrder(const Order& order)
{
    // Proceed only if the order is valid
    if (order.isValid()) {
        std::cout << order.sourcePlanet->planetID() << " " << order.destinationPlanet->planetID() << " " << order.shipsCount << std::endl;
        updateState(order);
    }
}

void Game::initializeState(const std::string &state)
{
    std::vector<std::string> lines = StringTokenizer::Tokenize(state, '\n');
    uint planetID = 0;

    uint linesSize = lines.size();
    for (uint i = 0; i < linesSize; ++i) {
        std::string line = lines.at(i);

        // Strip off comments from line
        size_t comment_begin = line.find_first_of('#');
        if (comment_begin != std::string::npos) {
            line = line.substr(0, comment_begin);
        }

        std::vector<std::string> tokens = StringTokenizer::Tokenize(line, ' ');
        uint tokensSize = tokens.size();
        if (tokensSize == 0) {
            continue;
        }
        // Create planets
        if (tokens.at(0) == "P" && tokensSize == 6) {
            uint  shipsCount   = atoi(tokens.at(4).c_str());
            uint  growthRate = atoi(tokens.at(5).c_str());
            uint  ownerID    = atoi(tokens.at(3).c_str());
            Point coordinate = Point(atof(tokens.at(1).c_str()), atof(tokens.at(2).c_str()));

            Player* owner = playerByID(ownerID);
            if (!owner) {
                owner = new Player(ownerID);
                players_m.push_back(owner);
                playersCount_m++;
            }

            Planet* newPlanet = new Planet(planetID, shipsCount, growthRate, coordinate, owner);
            planets_m.push_back(newPlanet);
            owner->addPlanet(newPlanet);

            planetID++;
        }
        // Create fleets
        else if (tokens.at(0) == "F" && tokensSize == 7) {
            uint ownerID        = atoi(tokens.at(1).c_str());
            uint shipsCount     = atoi(tokens.at(2).c_str());
            uint tripLength     = atoi(tokens.at(5).c_str());
            uint turnsRemaining = atoi(tokens.at(6).c_str());
            Planet* sourcePlanet      = planets_m.at(atoi(tokens.at(3).c_str()));
            Planet* destinationPlanet = planets_m.at(atoi(tokens.at(4).c_str()));

            Player* owner = playerByID(ownerID);
            if (!owner) {
                owner = new Player(ownerID);
                players_m.push_back(owner);
                playersCount_m++;
            }

            Fleet* newFleet = new Fleet(owner, sourcePlanet, destinationPlanet, shipsCount, tripLength, turnsRemaining);
            fleets_m.push_back(newFleet);

            // Add fleet to source and destination planet respectively
            sourcePlanet->addLeavingFleet(newFleet);
            destinationPlanet->addIncomingFleet(newFleet);
            owner->addFleet(newFleet);
        }
    }

    // Assign other planets to each planet
    planetsCount_m = planetID;
    for (uint i = 0; i < planetsCount_m; ++i) {
        planets_m.at(i)->setOtherPlanets(planets());
    }

    turn_m++;
}

void Game::updateState(const std::string& state)
{
    std::vector<std::string> lines = StringTokenizer::Tokenize(state, '\n');
    uint planetID = 0;

    deleteFleets();
    clearPlayersPlanets();

    uint linesSize = lines.size();
    for (uint i = 0; i < linesSize; ++i) {
        std::string line = lines.at(i);

        // Strip off comments from line
        size_t comment_begin = line.find_first_of('#');
        if (comment_begin != std::string::npos) {
            line = line.substr(0, comment_begin);
        }

        std::vector<std::string> tokens = StringTokenizer::Tokenize(line, ' ');
        uint tokensSize = tokens.size();
        if (tokensSize == 0) {
            continue;
        }
        // Update planets
        if (tokens.at(0) == "P" && tokensSize == 6) {
            uint  shipsCount   = atoi(tokens.at(4).c_str());
            uint  ownerID    = atoi(tokens.at(3).c_str());

            Player* owner = playerByID(ownerID);
            if (!owner) {
                owner = new Player(ownerID);
                players_m.push_back(owner);
                playersCount_m++;
            }
            Planet* updatedPlanet = planets_m.at(planetID);
            updatedPlanet->update(owner, shipsCount);
            owner->addPlanet(updatedPlanet);

            planetID++;
        }
        // Create fleets
        else if (tokens.at(0) == "F" && tokensSize == 7) {
            uint ownerID        = atoi(tokens.at(1).c_str());
            uint shipsCount     = atoi(tokens.at(2).c_str());
            uint tripLength     = atoi(tokens.at(5).c_str());
            uint turnsRemaining = atoi(tokens.at(6).c_str());
            Planet* sourcePlanet      = planets_m.at(atoi(tokens.at(3).c_str()));
            Planet* destinationPlanet = planets_m.at(atoi(tokens.at(4).c_str()));

            Player* owner = playerByID(ownerID);
            if (!owner) {
                owner = new Player(ownerID);
                players_m.push_back(owner);
                playersCount_m++;
            }

            Fleet* newFleet = new Fleet(owner, sourcePlanet, destinationPlanet, shipsCount, tripLength, turnsRemaining);
            fleets_m.push_back(newFleet);

            // Add fleet to source and destination planet respectively
            sourcePlanet->addLeavingFleet(newFleet);
            destinationPlanet->addIncomingFleet(newFleet);
            owner->addFleet(newFleet);
        }
    }

    turn_m++;
}

void Game::updateState(Order order)
{
    if (order.isValid()) {
        // Update number of ships on the source planet
        order.sourcePlanet->update(order.sourcePlanet->owner(), order.sourcePlanet->shipsCount() - order.shipsCount);

        Fleet* newFleet = new Fleet(order);
        fleets_m.push_back(newFleet);

        // Add fleet to source and destination planet respectively
        order.sourcePlanet->addLeavingFleet(newFleet);
        order.destinationPlanet->addIncomingFleet(newFleet);
    }
}

void Game::finishTurn() const {
    std::cout << "go" << std::endl;
    std::cout.flush();
}

