/* 
   Author: Moritz Gronbach
   Oct2010
   Main file for my Planet Wars bot for the Google AI Challenge
   General Architecture:
   A filtered list of possible orders is created. Only targets with certain properties, depending on the situation, are allowed. The best order according to a simple heuristic is chosen (see value()). The evaluation of an order takes into account current fleet movements. 
   This process is repeated maxActions times.
   Once this is done, the ships that are currently on planets and not needed for defense are sent towards the frontline.
   The most critical areas for performance are:
   1. the decision what constitutes a valid target
   2. the degree of aggression in the frontier-planet selection 
   3. precision of calculating ships needed to conquer neutral planets

   TODO:
   -remake parts of the frontier mechanic
*/

#include "MyBot.h"

#include <algorithm>
#include <map>
#include <vector>
#include <list>
#include <iostream>
#include <cmath>
#include <set>

#include "order.h"
#include "planet.h"
#include "player.h"
#include "knapsackTarget.h"

using std::min;
using std::max;
using std::vector;
using std::map;
using std::cerr;
using std::endl;

MyBot::MyBot(Game* game) :
    AbstractBot(game),
    logging(true)
{
}


//the entry function, this is called by the game engine
void MyBot::executeTurn() {
    if(logging){
        cerr << "#########################################" << endl;
        cerr << "Turn: " << game->turn() << endl;
    }
  
    if (game->turn() == 1) {
        initialize();
        preprocessing();
        myStartingPlanet = myPlanets[0];
        enemyStartingPlanet = enemyPlanets[0];
        if(logging){
            cerr << "MaxDistanceBetweenPlanets: " << maxDistanceBetweenPlanets << endl;
            cerr << "Distance between me and enemy: " << myStartingPlanet->distance(enemyStartingPlanet) << endl;
        }
    }

    preprocessing();

    if (game->turn() == 1){
        openingTurn();
        return;
    }



    //this prevents timeouts caused by a certain bug in the game engine:
    if (myPlanets.size() == 0 || enemyPlanets.size() ==0) { 
        return;
    } 


    const int maxActions = 5;
    for(int actions(0); actions != maxActions; ++actions) {
        //find a good action and execute it, if possible
        if (!chooseAction()) { 
            break; //stop if no good action can be found
        }
    }


    updatePredictions();

    if(logging){
        cerr << "checking for bad situation" << endl;
    }
    bool badSituation = me->shipsCount() < enemy->shipsCount()
        && game->turn() > maxDistanceBetweenPlanets
        && ((me->growthRate() < enemy->growthRate()
             && myPredictedGrowthRate(lookahead) < enemyPredictedGrowthRate(lookahead))
            || (turnsRemaining < maxDistanceBetweenPlanets*2
                && me->growthRate() <= enemy->growthRate()
                && myPredictedGrowthRate(lookahead) <= enemyPredictedGrowthRate(lookahead)));
    if(logging){
        cerr << "finished checking for bad situation" << endl;
    }
    if(badSituation){
        panic();
        updatePredictions();
    }
    //sent available ships towards the frontier
    supply();



   
    bool dominating = me->shipsOnPlanets() > enemy->shipsCount()*1.1 && enemyPlanets.size()>0;
    if(dominating){
        updatePredictions(); 
        int maxShips(0);
        Planet* maxPlanet;
        for(Planets::iterator pit = myPlanets.begin(); pit != myPlanets.end(); ++pit){
            Planet* p = *pit;
            int sc = p->shipsCount();
            if(sc>maxShips){
                maxShips = sc;
                maxPlanet = p;
            }
        }
        if(maxShips>0){
            Order o(maxPlanet, nearestEnemyPlanet(maxPlanet), min(maxShips, me->shipsCount() - 11*enemy->shipsCount()/10)/2);
            issueOrder(o, "domination");
        }
    }
    if(logging){
        cerr << "Turn finished" << endl;
    }
}


//this function is called only in turn 1, 
//and can be used for computing static properties of the map
void MyBot::initialize() {
    if(logging){
        cerr << "initializing..." << endl;
    }
    //calculate the size of the map, defined as the maximum distance between two planets
    maxDistanceBetweenPlanets = 1;
    const Planets planets = game->planets();
    for(Planets::const_iterator pit1 = planets.begin(); pit1!= planets.end();++pit1) {
        Planet* p1 = *pit1;
        for(Planets::const_iterator pit2 = planets.begin(); pit2!= planets.end();++pit2) {
            Planet* p2 = *pit2;
            int dist = p1->distance(p2);
            if (dist > maxDistanceBetweenPlanets) {
                maxDistanceBetweenPlanets = dist;
            }
        }
    }
    enemy = game->playerByID(2);
    me = game->playerByID(1);
    if(logging){
        cerr << "max distance between planets: " << maxDistanceBetweenPlanets << endl;
        cerr << "initializing finished" << endl;
    }
}


//this updates the things that change from turn to turn, and makes predictions on planets future states
void MyBot::preprocessing() {
    if(logging){
        cerr << "preprocessing..." << endl;
    }
    const int turnLimit = 200; //according to the contest organizators, this will probably continue to be the turn limit until the end of the contest.
    turnsRemaining = turnLimit - game->turn();
    myPlanets = game->myPlanets();
    notMyPlanets = game->notMyPlanets();
    enemyPlanets = game->enemyPlanets();
    neutralPlanets = game->neutralPlanets();
    planets = game->planets();
    lookahead = maxDistanceBetweenPlanets;
  
    updatePredictions();
    for(Planets::const_iterator pit = myPlanets.begin();pit!=myPlanets.end();++pit) {
        Planet* p = *pit;
        if(logging){
            cerr << p->planetID() << ": comp: " << shipsAvailable(competitivePredictions[p], lookahead) << "; static: " << shipsAvailable(predictions[p], lookahead) <<  endl;
        }
    }
    if(logging){
        cerr << "preprocessing finished" << endl;
    }
}


void MyBot::updatePredictions(){
    if(logging){
        cerr << "updating predictions..." << endl;
    }
    predictions.clear();
    competitivePredictions.clear();
    worstCasePredictions.clear();
    maxOutgoingFleets.clear();
    //static predictions taking into account only current fleet movements
    for(Planets::const_iterator pit = planets.begin();pit!=planets.end();++pit) {
        Planet* p = *pit;
        predictions[p] = p->getPredictions(lookahead);
    }
    if(logging){
        cerr << "updated static predictions..." << endl;
    }

    for(Planets::const_iterator pit = planets.begin();pit!=planets.end();++pit) {
        Planet* p = *pit;
        maxOutgoingFleets[p] = computeMaxOutgoingFleets(p);
    }
    if(logging){
        cerr << "updated max outgoing fleets..." << endl;
    }

    //special-case predictions for each planet. It is important that the static predictions happen before this.
    for(Planets::const_iterator pit = planets.begin();pit!=planets.end();++pit) {
        Planet* p = *pit;
        worstCasePredictions[p] = p->getPredictions(lookahead, worstCaseFleets(p));
        competitivePredictions[p] = p->getPredictions(lookahead, competitiveFleets(p));
    }
    if(logging){
        cerr << "updated competitive and worst-case predictions..." << endl;
    }


    //determine which planets I could conquer according to current predictions.
    //These planets are candidates for frontier planets, even though I don't own them yet
    for(Planets::const_iterator pit = planets.begin();pit!=planets.end();++pit) {
        Planet* p = *pit;
        p->predictedMine = false;
        p->predictedEnemy = false;
        for(int i(0); i != lookahead + 1; ++i){
            if (predictions[p][i].owner()->isMe()) {
                p->predictedMine = true;
            }
            if (predictions[p][i].owner()->isEnemy()) {
                p->predictedEnemy = true;
            }
        }
    }
    if(logging){
        cerr << "updated \"predicted mine\"..." << endl;
    }

    for(Planets::const_iterator pit = planets.begin();pit!=planets.end();++pit) {
        Planet* p = *pit;
        p->frontierStatus = isFrontier(p);
    }
    if(logging){
        cerr << "updated frontier planets..." << endl;
    }

    myPredictedGrowth = myPredictedGrowthRate(lookahead); 
    enemyPredictedGrowth = enemyPredictedGrowthRate(lookahead);
    if(logging){
        cerr << "set predicted growth rates..." << endl;
    }
    if(logging){
        cerr << "updating predictions finished" << endl;
    }
}

//this is executed in the opening of the game. 
//Tries to allocate ships in a way that maximizes growth (by solving a knapsack01-problem), while not exposing the starting planet.
void MyBot::openingTurn() {
    if(logging){
        cerr << "opening turn started..." << endl;
    }
    vector<KnapsackTarget> candidates;
    for(Planets::const_iterator pit = planets.begin(); pit != planets.end(); ++pit) {
        Planet* p = *pit;
        if (!predictions[p][lookahead].owner()->isMe() && p->distance(myStartingPlanet) < p->distance(enemyStartingPlanet)) {
            KnapsackTarget kt;
            kt.p = predictions[p][p->distance(myStartingPlanet)];
            kt.weight = kt.p.shipsCount() + 1;
            candidates.push_back(kt);
            if(logging){
                cerr << "added knapsack candidate: planet " << p->planetID() << endl;
            }
        }
    }
    for(vector<KnapsackTarget>::iterator kt = candidates.begin(); kt != candidates.end(); ++kt) {
        kt->value = kt->p.growthRate()*(maxDistanceBetweenPlanets - kt->p.distance(myStartingPlanet)); 
    }
    
    int sa = shipsAvailable(competitivePredictions[myStartingPlanet], lookahead);
    int maxWeight = sa;
      
    vector<KnapsackTarget> targets = knapsack01(candidates,maxWeight);
    if(logging){
        cerr << "Knapsack01 found " << targets.size() << " targets" << endl;
    }
    for(vector<KnapsackTarget>::const_iterator kt = targets.begin(); kt != targets.end(); ++kt) {
        if(logging){
            cerr << "knapsack target: " << kt->p.planetID() << endl;
        }
        Planet* p = game->planetByID(kt->p.planetID());
        if(kt->weight <= sa){
            Order o(myStartingPlanet, p, kt->weight); 
            issueOrder(o, "opening turn knapsack move.");
        }
    }
    if(logging){
        cerr << "opening turn finished" << endl;
    }
}


//tries to find a good action and executes it, returns true on success.
bool MyBot::chooseAction() {
    if(logging){
        cerr << "started chooseAction..." << endl;
    }
    vector<Orders> orderCandidates;
    for(Planets::const_iterator p1 = myPlanets.begin(); p1!= myPlanets.end(); ++p1) {
        Planet* source1 = *p1;
        int i(0);
        for(Planets::const_iterator p2 = myPlanets.begin(); p2!= myPlanets.end() && i<10; ++p2) {
            Planet* source2 = *p2;
            if(source1->planetID() <= source2->planetID()){
                addOrderCandidates(source1, source2, orderCandidates);
                ++i;
            }
        }
    }
    if(logging){
        cerr << "added all action candidates (" << orderCandidates.size() << ")" << endl;
    }
    //find the best action candidate:
    int maxValue = 0;
    Orders* maxOrders(0);
    for(vector<Orders>::iterator oit = orderCandidates.begin(); oit != orderCandidates.end(); ++oit) {
        Orders* o=&*oit;
        int newValue = value(*o, false);
        if(logging){
            cerr << "evaluated the following orders: " << endl;
            logOrders(*o);
            cerr << "Value: " << newValue << endl;
        }
        if (newValue>maxValue) {
            maxValue = newValue;
            maxOrders = o; 
        }
    }
    
    //execute the order with the highest value:
    if (maxValue>0 && maxOrders) {
        if(logging){
            cerr << "found best action, optimizing..." << endl;
        }
//        issueOrder(optimizeOrder(maxOrder), "action evaluation");
        issueOrders(optimizeOrders(*maxOrders));
        updatePredictions();
        if(logging){
            cerr << "finished chooseAction" << endl;
        }
        return true;
    } else {
        if(logging){
            cerr << "finished chooseAction, found none" << endl;
        }
        return false;
    }
}

void MyBot::logOrders(const Orders& os){
    for(Orders::const_iterator oit = os.begin(); oit != os.end(); ++oit){
        cerr << oit->sourcePlanet->planetID() << " " << oit->destinationPlanet->planetID() << " " << oit->shipsCount << endl;
    }
}

void MyBot::issueOrders(const Orders& os){
    if(logging){
        cerr << "issuing " << os.size() << " orders" << endl;
    }
    for(Orders::const_iterator oit = os.begin(); oit != os.end(); ++oit){
        issueOrder(*oit, "action evaluation");
    }
}

Orders MyBot::optimizeOrders(const Orders& os){
          
    Planet* source1 = os[0].sourcePlanet;
    Planet* source2 = os[1].sourcePlanet;
    Planet* dest = os[0].destinationPlanet;
    int dist = max(dest->distance(source1), dest->distance(source2));
    Orders osMax;
    osMax.push_back(Order(source1, dest, 0));
    osMax.push_back(Order(source2, dest, 0));
    int maxVal(0);
    int sa1 = os[0].shipsCount;
    int sa2 = os[1].shipsCount;
    /*if(source1->frontierStatus){
        sa1 = min(shipsAvailable(competitivePredictions[source1], 2 * dist), sa1);
    }
    if(source2->frontierStatus){
        sa2 = min(shipsAvailable(competitivePredictions[source2], 2 * dist), sa2);
    }
    if(os[0].shipsCount == 0){
        sa1 = 0;
    }
    if(os[1].shipsCount == 0){
        sa2 = 0;
        }*/

    int divs1(20);
    int divs2(20);
    if(sa1==0){
        divs2 = sa2;
        divs1 = 0;
    }   
    if(sa2==0){
        divs1 = sa1;
        divs2 = 0;
    }
    if(logging){
        cerr << "sa: " << sa1 << ", " << sa2 << endl; 
    }
    for(int i(0); i <= divs1; ++i){
        for(int j(0); j <= divs2 ; ++j){
            Orders ovec;
            ovec.push_back(Order(source1, dest, divs1>0 ? (i * sa1) / divs1 : 0));
            ovec.push_back(Order(source2, dest, divs2>0 ? (j * sa1) / divs2 : 0));
            int val = value(ovec);
            if(logging){
                cerr << i << ", " << j << ": " << val << endl;
            }
            if(val>maxVal){
                osMax[0].shipsCount = i;
                osMax[1].shipsCount = j;
                maxVal = val;
            } 
        }
    }
    if(maxVal>0){
        return osMax;
    } else {
        return os;
    }
}
      
   


//manages the supply chain
void MyBot::supply() {
    if(logging){
        cerr << "supply started..." << endl;
    }
    updatePredictions();
    //find attractive neutral planets, so there will be sent ships towards them via the supply lines:
    setExpansionTargets();
  
    //compute available ships for each planet, i.e. ships that are not needed for defense according to the static predictions
    map<Planet*,int> shipsAvail;
    for(Planets::const_iterator pit = myPlanets.begin(); pit != myPlanets.end(); ++pit) {
        Planet* p = *pit;
        if (p->frontierStatus) {
            shipsAvail[p] = 0;
        } else {
            shipsAvail[p] = shipsAvailable(predictions[p],lookahead);
            if(shipsAvail[p] < 0){
                p->frontierStatus = true;
                if(logging){
                    cerr << "Planet " << p->planetID() << " is in peril and has been set as frontier" << endl;
                }
            }
        }
    }
    


    //send available ships to the frontier
    for(Planets::const_iterator pit = myPlanets.begin(); pit != myPlanets.end(); ++pit) {
        Planet* p = *pit;
        if (shipsAvail[p]>0) {
            Planet* target = game->planetByID(supplyMove(p, nearestFrontierPlanet(p)));
            if (target->owner()->isMe() 
                || target->predictedMine) {
                Order o(p, target, shipsAvail[p]);
                issueOrder(o, "supply to own planet");
                shipsAvail[p] = 0;
            } else if(competitivePredictions[target][p->distance(target)].owner()->isMe() &&  competitivePredictions[target][lookahead].owner()->isMe()){
                Order o(p, target, shipsAvail[p]);
                issueOrder(o, "supply, hope to gain");
                shipsAvail[p] = 0;
            } else {
                if(logging){
                    cerr << "Planet " << p->planetID() << " did not supply his frontier planet " << target->planetID() << endl;
                }
            }
        }
    }


    if(logging){
        cerr << "supply ended" << endl;
    }
}


void MyBot::panic(){
    if(logging){
        cerr << "panicking!" << endl;
    }
    for(Planets::const_iterator pit = myPlanets.begin(); pit != myPlanets.end(); ++pit){
        Planet* p = *pit;
        if(p->frontierStatus){
            Planets closest = p->closestPlanets();
            for(Planets::const_iterator pit2 = closest.begin(); pit2 != closest.end(); ++pit2){
                Planet* p2 = *pit2;
                if(p2->owner()->isEnemy()){
                    int dist = p->distance(p2);
                    int shipsRequired = predictions[p2][dist].shipsCount() + 1;
                    if(p->shipsCount() >= shipsRequired){
                        Order o(p, p2, shipsRequired);
                        issueOrder(o, "panic");
                    }
                }
            }
        }
    }
    if(logging){
        cerr << "panicking finished" << endl;
    }
}

//find the order candidates for planets sourcei. Orders are added by modifying the passed-by-ref orderCandidates.
//there is a lot of finetuning for special cases in this function, that makes it somewhat ugly.
void MyBot::addOrderCandidates(Planet* source1, Planet* source2, vector<Orders>& orderCandidates){
    if(logging){
        cerr << "adding order candidates for planets " << source1->planetID() << " and " << source2->planetID() << "..." <<  endl;
    }
    int shipsAvailableStatic1 = shipsAvailable(predictions[source1],lookahead);
    if (!source2->frontierStatus && shipsAvailableStatic1 < 0 && willHoldFor(predictions[source1], 0) < source1->distance(nearestFriendlyPlanet(source1))) {
        shipsAvailableStatic1 = source1->shipsCount();
    }
    int shipsAvailableStatic2 = shipsAvailable(predictions[source2],lookahead);
    if (!source2->frontierStatus && shipsAvailableStatic2 < 0 && willHoldFor(predictions[source2], 0) < source2->distance(nearestFriendlyPlanet(source2))) {
        shipsAvailableStatic2 = source2->shipsCount();
    }
    for(Planets::const_iterator p = planets.begin(); p!= planets.end(); ++p) {
        Planet* destination = *p;
        int dist = max(source1->distance(destination), source2->distance(destination));
        if(destination != source1 && destination != source2 && dist <= turnsRemaining && dist <= maxDistanceBetweenPlanets / 2 && abs(source1->distance(destination)-source2->distance(destination)) < maxDistanceBetweenPlanets / 4 && !(destination->owner()->isMe() && predictions[destination][lookahead].owner()->isMe()) && !(me->growthRate() > enemy->growthRate() && myPredictedGrowth > enemyPredictedGrowth && destination->owner()->isNeutral() && predictions[destination][lookahead].owner()->isNeutral())){
            int shipsAvailableCompetitive1 = shipsAvailable(competitivePredictions[source1], dist*2);
            int shipsAvailableCompetitive2 = shipsAvailable(competitivePredictions[source2], dist*2);
            if (max(shipsAvailableStatic1, shipsAvailableCompetitive1)>0 && max(shipsAvailableStatic2, shipsAvailableCompetitive2)>0) {
                Planet futureDestination = predictions[destination][dist];
                int shipsRequired = futureDestination.shipsCount()+1;
                if (futureDestination.owner()->isMe()) {
                    for(int t(dist); t!=lookahead+1; ++t) {
                        if (predictions[destination][t].owner()->isEnemy()) {
                            shipsRequired = predictions[destination][t].shipsCount()+1;
                            break;
                        }
                    }
                    continue;
                }
                int shipsRequiredWorstCase = worstCasePredictions[destination][dist].shipsCount()+1;
                //the conditions for validity make sure that my bot is not too aggressive, and does not attack neutrals when the enemy can snipe
                bool valid =  shipsRequired <= min(shipsAvailableCompetitive1, shipsAvailableStatic1) + min(shipsAvailableCompetitive2, shipsAvailableStatic2) 
                    &&!(futureDestination.owner()->isNeutral() 
                        && me->shipsCount()*2 < enemy->shipsCount() ) 
                    && !(futureDestination.owner()->isEnemy() 
                         && predictions[destination][dist-1].owner()->isNeutral()) 
                    && (competitivePredictions[destination][dist].owner()->isMe() 
                        || competitivePredictions[destination][min(lookahead, 2*dist)].owner()->isMe()
                        || (me->growthRate() < enemy->growthRate() 
                            && myPredictedGrowth < enemyPredictedGrowth)
                        || dist <= maxDistanceBetweenPlanets/6);
                if (valid) {
                    Orders ovec;
                    vector<int> source1SC;
                    int minSA1 = min(shipsAvailableStatic1, shipsAvailableCompetitive1);
                    int maxSA1 = max(shipsAvailableStatic1, shipsAvailableCompetitive1);
                    int minSA2 = min(shipsAvailableStatic2, shipsAvailableCompetitive2);
                    int maxSA2 = max(shipsAvailableStatic2, shipsAvailableCompetitive2);
                    int srHalf = shipsRequired / 2 + 1;
                    if(maxSA1>=srHalf && !(source1->frontierStatus && minSA1 < srHalf))
                        source1SC.push_back(srHalf);
                    if(maxSA1>=shipsRequired && !(source1->frontierStatus && minSA1 < (shipsRequired)))
                        source1SC.push_back(shipsRequired);
                    if(minSA1>0 )
                        source1SC.push_back(minSA1);
                    /*                  if(maxSA1>0 && !source1->frontierStatus) 
                                        source1SC.push_back(maxSA1);*/
                    vector<int> source2SC;
                    if(source1 == source2){
                        source2SC.push_back(0);
                    } else {
                        if(maxSA1>=srHalf && !(source2->frontierStatus && minSA2 < srHalf))
                            source2SC.push_back(srHalf);
                        if(maxSA1>=shipsRequired && !(source2->frontierStatus && minSA2 < (shipsRequired)))
                            source2SC.push_back(shipsRequired);
                        if(minSA2>0)
                            source2SC.push_back(minSA2);
                    }
/*                    if(maxSA2>0 && !source2->frontierStatus) 
                        source2SC.push_back(maxSA2);*/

                    for(vector<int>::iterator scit1 = source1SC.begin(); scit1 != source1SC.end(); ++scit1){
                        for(vector<int>::iterator scit2 = source2SC.begin(); scit2 != source2SC.end(); ++scit2){
                            if(*scit1 != 0 || *scit2 != 0){
                                ovec.push_back(Order(source1, destination, *scit1));
                                ovec.push_back(Order(source2, destination, *scit2));
                                orderCandidates.push_back(ovec);
                                ovec.clear();
                            }
                        }
                    }                       

                    if(logging){
                        cerr << "considering planet " << destination->planetID() <<endl;
                    }
                }
        
            }
        }
    }
    if(logging){
        cerr << "finished adding order candidates for planets" << endl;
    }
}


list<Fleet> MyBot::ordersToFleets(const Orders& os){
    list<Fleet> fs;
    for(Orders::const_iterator oit = os.begin(); oit != os.end(); ++oit){
        int dist = oit->sourcePlanet->distance(oit->destinationPlanet);
        fs.push_back(Fleet(oit->sourcePlanet->owner(), oit->sourcePlanet, oit->destinationPlanet, oit->shipsCount, dist, dist));
    }
    return fs;
}

int MyBot::value(const Orders& os, bool worstcase){
    set<Planet*> sources;
    for(Orders::const_iterator oit = os.begin(); oit != os.end(); ++oit){
        sources.insert(oit->sourcePlanet);
    }
    Planet* destination = os[0].destinationPlanet;  //I assume that all sources have the same destination
    list<Fleet> fs(ordersToFleets(os));
    int baseValue(0);
    int dist(0);
    for(set<Planet*>::const_iterator pit = sources.begin(); pit != sources.end(); ++pit){
        Planet* p = *pit;
        dist = max(dist, destination->distance(p));
        baseValue += value(predictions[p]);
    }

    baseValue += value(predictions[destination]);



   
    if (predictions[destination][dist].owner()->isNeutral()) {  //some tougher payoff conditions for neutral planets
        list<Fleet> wfsDest = worstCaseFleets(destination);
        for(list<Fleet>::iterator fit = wfsDest.begin(); fit != wfsDest.end(); ++fit){
            fit->turnsRemaining_m = max(dist+1, int(fit->turnsRemaining_m));
            if(fit->turnsRemaining_m > maxDistanceBetweenPlanets / 2){
                fit = wfsDest.erase(fit);
                if(fit == wfsDest.end())
                    break;
            }
        } 
        wfsDest.insert(wfsDest.end(), fs.begin(), fs.end());
        for(set<Planet*>::const_iterator pit = sources.begin(); pit != sources.end(); ++pit){
            Planet* p = *pit;
            for(list<Fleet>::const_iterator fit = maxOutgoingFleets[p].begin(); fit != maxOutgoingFleets[p].end(); ++fit){
                if(fit->owner()->isMe() && fit != maxOutgoingFleets[p].begin()){
                    wfsDest.push_back(*fit);
                }
            }
        }
        //a neutral planet has to pay off even in the close-to worst case:
        vector<Planet> preds = destination->getPredictions(lookahead,wfsDest);
        int whf = willHoldFor(preds, dist)*destination->growthRate();
        if ( whf  < destination->shipsCount()) {
            if(logging){
                cerr << ">>" << destination->planetID() << " does not fulfill payout conditions for neutrals: " << whf << "<" << destination->shipsCount() << endl;
            }
            return 0;
        } 
    }
    vector<Planet> destinationPredictions = destination->getPredictions(lookahead, fs);
    int newValue = value(destinationPredictions) - baseValue;
    for(set<Planet*>::const_iterator pit = sources.begin(); pit != sources.end(); ++pit){
        Planet* p = *pit;
        newValue += value(p->getPredictions(lookahead,fs));
    }
    //higher distance means that the payoff predictions are less reliable, so closer targets are preferable:
    newValue *= (maxDistanceBetweenPlanets - dist + 1);  
    return newValue;
}

  

//find neutral planets that are interesting targets for expansion, and enemy planets that I might be able to conquer
//TODO: similar for enemy planets
void MyBot::setExpansionTargets(){
    //if there is a neutral planet that I might be able to conquer, and that pays off fast enough, make this neutral a frontier planet
    int fastestPayoff = lookahead;
    Planet* fastestPayoffPlanet(0);
    for(Planets::const_iterator pit = neutralPlanets.begin(); pit != neutralPlanets.end(); ++pit) {
        Planet* p = *pit;
        Planets temp = myPlanets;
        temp.push_back(p);
        if (p->growthRate()>0 
            && (predictions[p][lookahead].owner()->isNeutral() 
                && competitivePredictions[p][lookahead].owner()->isMe() 
                && (me->shipsCount() 
                    + me->growthRate() * distance(enemyPlanets,temp) 
                    - predictions[p][lookahead].shipsCount() 
                    >= enemy->shipsCount()) 
                && (me->growthRate() < enemy->growthRate() 
                    || myPredictedGrowth < enemyPredictedGrowth))) {
            int timeToPayoff = predictions[p][lookahead].shipsCount() / p->growthRate();
            if (timeToPayoff < fastestPayoff) {
                fastestPayoff = timeToPayoff;
                fastestPayoffPlanet = p;
            }
        }
    }
    if (fastestPayoff < maxDistanceBetweenPlanets && fastestPayoffPlanet) {
        fastestPayoffPlanet->frontierStatus = true;
        if(logging){
            cerr << "Set expansion target: " << fastestPayoffPlanet->planetID() << endl;
        }
        for(Planets::const_iterator pit = planets.begin(); pit != planets.end(); ++pit) {
            Planet* p = *pit;
            if (p != fastestPayoffPlanet &&  protects(fastestPayoffPlanet, p)) {
                int dist = p->distance(fastestPayoffPlanet);
                p->frontierStatus = false;
                int sa = min(shipsAvailable(predictions[p], dist), shipsAvailable(competitivePredictions[p], dist));
                issueOrder(Order(p, fastestPayoffPlanet, sa), "quick supply");
            }
        }
    }
  

    for(Planets::const_iterator pit = planets.begin(); pit != planets.end(); ++pit) {
        Planet* p = *pit;
        if ((p->owner()->isEnemy() || predictions[p][lookahead].owner()->isEnemy()) && competitivePredictions[p][lookahead].owner()->isMe()) {
            p->frontierStatus = true;
            for(Planets::const_iterator pit2 = planets.begin(); pit2 != planets.end(); ++pit2) {
                Planet* p2 = *pit;
                if (p2->planetID() != p->planetID() &&  protects(p, p2) && p2->owner()->isMe()) {
                    int dist = p->distance(fastestPayoffPlanet);
                    p->frontierStatus = false;
                    int sa = min(shipsAvailable(predictions[p], dist), shipsAvailable(competitivePredictions[p], dist));
                    issueOrder(Order(p, fastestPayoffPlanet, sa), "quick supply");
                }
            }
        }
    }


}

//evaluates a list of predictions for a planet. This is used to calculate the value of an action/order.
int MyBot::value(const vector<Planet>& preds) const{
    int factor = 0;
    Planet finalPlanet = preds[min(lookahead,turnsRemaining)];
    if (finalPlanet.owner()->isMe()) {
        factor = 1;
    } else if (finalPlanet.owner()->isEnemy()) {
        factor = -1;
    }
    return finalPlanet.shipsCount() * factor;
}

list<Fleet> MyBot::computeMaxOutgoingFleets(Planet* pl){
    list<Fleet> outFleets;
    Planet p = *pl;
    if(!p.owner()->isNeutral()){
        Fleet f(p.owner(), p.shipsCount());
        outFleets.push_back(f);
        p.shipsCount_m = 0;
    } else {
        outFleets.push_back(Fleet(p.owner(), 0));
    }
    for(int i(0); i != lookahead+1; ++i){
        p = p.getPredictions(1,i)[1];
        if(!p.owner()->isNeutral()){
            outFleets.push_back(Fleet(p.owner(), p.shipsCount()));
            p.shipsCount_m = 0;
        } else {
            outFleets.push_back(Fleet(p.owner(), 0));
        }
    }
    return outFleets;
}
  


//the following functions define the "protects" relationship and some utility function. 
//Planet A protects planet B from planet C, iff B can safely send ships to A, and A is closer to B than C. 
bool MyBot::protects(Planet* protector, Planet* protectee, Planet* from) const{
    return (from->distance(protector) < from->distance(protectee) 
            && 2*protectee->distance(protector) <= from->distance(protectee));
}


//Planet A protects planet B, if A protects B from all enemy planets.
bool MyBot::protects(Planet* protector, Planet* protectee) const{
    Planets closest = protectee->closestPlanets();
    int dist = protector->distance(protectee);
    for(Planets::const_iterator pit = closest.begin(); pit != closest.end(); ++pit) {
        Planet* p = *pit;
        if (p->distance(protectee) >= 2 * dist) {
            return true;
        }
        if (p->owner() != protectee->owner() 
            && !p->owner()->isNeutral() 
            && !protects(protector, protectee, p)) {
            return false;
        }
    }
    return true;
}

//returns true iff there is a planet that has the same owner as pl, and protects pl from Planet 'from'.
bool MyBot::isProtectedFrom(Planet* pl, Planet* from) const{
    Planets closest = pl->closestPlanets();
    int dist = from->distance(pl);
    for(Planets::const_iterator pit = closest.begin(); pit != closest.end(); ++pit) {
        Planet* p = *pit;
        if (p->owner() == pl->owner() && protects(p, pl, from)) {
            return true;
        }
        if (p->distance(pl) > dist) {
            return false;
        }
    }
    return false;
}

Planet* MyBot::isProtectedFromBy(Planet* pl, Planet* from) const{
    Planets closest = pl->closestPlanets();
    int dist = from->distance(pl);
    for(Planets::const_iterator pit = closest.begin(); pit != closest.end(); ++pit) {
        Planet* p = *pit;
        if (p->owner() == pl->owner() && protects(p, pl, from)) {
            return p;
        }
        if (p->distance(pl) > dist) {
            return pl;
        }
    }
    return pl;
}


//returns true iff planet pl is protected from all enemy planets
bool MyBot::isProtected(Planet* pl) const{
    Planets closest = pl->closestPlanets();
    for(Planets::const_iterator pit = closest.begin(); pit != closest.end(); ++pit) {
        Planet* p = *pit;
        if (p->owner() != pl->owner() && !p->owner()->isNeutral()) {
            bool protectedFromp(false);
            for(Planets::const_iterator pit2 = closest.begin(); pit2 != closest.end(); ++pit2) {
                Planet* p2 = *pit2;
                if (p2->owner()->isMe() && protects(p2,pl,p)) {
                    protectedFromp = true;
                    break;
                }
            }
            if (!protectedFromp) {
                return false;
            }
        }
    }
    return true;
}


//determines if planet pl is one of my frontier planets.
//Simplified, pl is a frontier planet, if there is an enemy planet e, such that pl minimizes the distance from my planets to e.
bool MyBot::isFrontier(Planet* pl) const{
    if (!(pl->owner()->isMe() || pl->predictedMine)) {
        return false;
    }
    Planets closest = pl->closestPlanets();
    for(Planets::iterator pit = closest.begin(); pit != closest.end(); ++pit) {
        Planet* p = *pit;
        if (!p->predictedMine && (p->owner()->isEnemy() || p->predictedEnemy)) {
            Planets p_closest = p->closestPlanets();
            for(Planets::iterator pit2 = p_closest.begin(); pit2 != p_closest.end(); ++pit2) {
                Planet* p2 = *pit2;
                if (p2->planetID() == pl->planetID()) {
                    if(logging){
                        cerr << "Planet " << pl->planetID() << " has been set as Frontier by isFrontier()" << endl;
                    }
                    return true;
                }
                if ((p2->owner()->isMe() || p2->predictedMine)  && p2->distance(pl) < pl->distance(p)) {
                    return false;
                }
            }
        }
    }
    return false;
}


//returns my next frontier planet
Planet* MyBot::nearestFrontierPlanet(Planet* pl) const{
    if (pl->frontierStatus)
        return pl;
    Planets closest = pl->closestPlanets();
    for(Planets::iterator pit = closest.begin(); pit != closest.end(); ++pit) {
        Planet* p = *pit;
        if (p->frontierStatus)
            return p;
    }
    return pl;
}


Planet* MyBot::nearestEnemyPlanet(Planet* pl) const{
    if(pl->owner()->isEnemy()){
        return pl;
    }
    Planets closest = pl->closestPlanets();
    for(Planets::iterator pit = closest.begin(); pit != closest.end(); ++pit) {
        Planet* p = *pit;
        if (p->owner()->isEnemy())
            return p;
    }
    return pl;
}

Planet* MyBot::nearestFriendlyPlanet(Planet* pl) const{
    Planets closest = pl->closestPlanets();
    for(Planets::iterator pit = closest.begin(); pit != closest.end(); ++pit) {
        Planet* p = *pit;
        if (p->owner()->isMe())
            return p;
    }
    return pl;
}

//how far is planet pl away from my frontier?
int MyBot::distanceToFrontier(Planet* pl) const{
    return pl->distance(nearestFrontierPlanet(pl));
}


//what's the closest planet to pl that is closer to the frontier than pl?
Planet* MyBot::nextPlanetCloserToFrontier(Planet* pl) const{
    if (pl->frontierStatus)
        return pl;                
    int dist = distanceToFrontier(pl);
    Planets closest = pl->closestPlanets();
    for(Planets::iterator pit = closest.begin(); pit != closest.end(); ++pit) {
        Planet* p = *pit;
        if (p->owner() == pl->owner() && distanceToFrontier(p)<dist)
            return p;
    }
    return pl;
}

//computes the minimum distance between two sets of planets. 
int MyBot::distance(const Planets& ps1, const Planets& ps2) const{
    int dist = ps1[0]->distance(ps2[0]);
    for(Planets::const_iterator pit = ps1.begin(); pit != ps1.end(); ++pit) {
        Planet* p = *pit;
        dist = min(p->distance(ps2), dist);
    }
    return dist;
}

//this computes the fleets for the scenario where both players send all their ships to planet pl.
//Necessary for predicting if it is possible for me to conquer the planet
list<Fleet> MyBot::competitiveFleets(Planet* pl) {
    list<Fleet> fs;
    Planets closest = pl->closestPlanets();
    for (Planets::const_iterator pit = closest.begin(); pit != closest.end(); ++pit) {
        Planet* p = *pit;
        int dist = p->distance(pl);
        list<Fleet>* maxOutgoing = &maxOutgoingFleets[p];
        int i(0);
        for(list<Fleet>::const_iterator fit = maxOutgoing->begin(); fit != maxOutgoing->end(); ++fit){
            if(!fit->owner()->isNeutral()){
                Fleet f2(fit->owner(), p, pl, fit->shipsCount(), dist+i, dist+i);
                fs.push_back(f2);
            }
            ++i;
        }
    }
    return fs;
}


//this computes the fleets in the scenario where the enemy sends all his ships to planet pl, and I send none.
//TODO: get rid of code duplication with competitiveFleets()
list<Fleet> MyBot::worstCaseFleets(Planet* pl) {
    list<Fleet> fs;
    vector<int> shipsInTurn;
    for(int i(0); i != lookahead+1+maxDistanceBetweenPlanets;++i) shipsInTurn.push_back(0);
    Planets closest = pl->closestPlanets();
    for (Planets::const_iterator pit = closest.begin(); pit != closest.end(); ++pit) {
        Planet* p = *pit;
        int dist = p->distance(pl);
        list<Fleet>* maxOutgoing = &maxOutgoingFleets[p];
        int i(0);
        for(list<Fleet>::const_iterator fit = maxOutgoing->begin(); fit != maxOutgoing->end(); ++fit){
            if(fit->owner()->isEnemy()){
                shipsInTurn[dist+i] += fit->shipsCount();
            }
            ++i;
        }
    }
    for(int i(0); i != shipsInTurn.size(); ++i){
        Fleet f2(enemy, planets[0], pl, shipsInTurn[i], i, i);
        fs.push_back(f2);
    }
    return fs;
}
 
//Given predictions of a planets future, compute how long I will hold this planet starting at time t. 
//This is used to check if conquering a neutral planet pays off in the worst case.
int MyBot::willHoldFor(const vector<Planet>& predictions,int t) const{
    if(t >= predictions.size()){
        return 0;
    }
    for ( vector<Planet>::const_iterator pFuture = predictions.begin()+t; pFuture != predictions.end(); ++pFuture) {
        if (pFuture->owner()->isEnemy()) return pFuture - (predictions.begin()+t) -1;
    }
    return turnsRemaining;
}
    

//A dynamic programming solution to the knapsack01-problem.
//This is used in the first turn to allocate ships in a way that maximizes the production rate.
vector<KnapsackTarget> MyBot::knapsack01(const vector<KnapsackTarget>& candidates, int maxWeight) const{
    vector<int> weights;
    vector<int> values;

    for (vector<KnapsackTarget>::const_iterator kt = candidates.begin(); kt != candidates.end(); ++kt) {
        weights.push_back(kt->weight);
        values.push_back(kt->value);
    }

    int K[weights.size()+1][maxWeight];

    for (int i = 0; i < maxWeight; i++) {
        K[0][i] = 0;
    }
    for (int k = 1; k <= weights.size(); k++) {
        for (int y = 1; y <= maxWeight; y++) {
            if (y < weights[k-1]) {
                K[k][y-1] = K[k-1][y-1];
            } else if (y > weights[k-1]) {
                K[k][y-1] = max(K[k-1][y-1], K[k-1][y-1-weights[k-1]] + values[k-1]);
            } else {K[k][y-1] = max(K[k-1][y-1], values[k-1]);}
        }
    }

    int i = weights.size();
    int currentW = maxWeight-1;
    vector<KnapsackTarget> markedTargets;

    while ((i > 0) && (currentW >= 0)) {
        if (((i == 0) && (K[i][currentW] > 0)) || (K[i][currentW] != K[i-1][currentW])) {
            markedTargets.push_back(candidates[i-1]);
            currentW = currentW - weights[i-1];
        }
        i--;
    }
    return markedTargets;
}



//this computes the next planet in the supply chain starting in planet pl and ending in goal.
//I use A* for this, with dist*sqrt(dist) as cost function. This cost function favours smaller steps, which are needed for flexibility.
//The code for this function is mostly taken from http://code.google.com/p/a-star-algorithm-implementation/
int MyBot::supplyMove(Planet* pl, Planet* goal) {
    AStarSearch<Planet> astarsearch;
    Planet* ret;

    // Create a start state
    Planet nodeStart(*pl);


    // Define the goal state
    Planet nodeEnd(*goal);
		
		
    astarsearch.SetStartAndGoalStates( nodeStart, nodeEnd );

    unsigned int SearchState;
    unsigned int SearchSteps = 0;

    do
    {
        SearchState = astarsearch.SearchStep();

        SearchSteps++;

    }
    while( SearchState == AStarSearch<Planet>::SEARCH_STATE_SEARCHING );


    if ( SearchState == AStarSearch<Planet>::SEARCH_STATE_SUCCEEDED )
    {

        Planet *node = astarsearch.GetSolutionStart();

        ret = astarsearch.GetSolutionNext();

        astarsearch.FreeSolutionNodes();
        astarsearch.EnsureMemoryFreed();
        if (ret) {
            return ret->planetID();
        } else {
            return pl->planetID();
        }
	
    }
    else if ( SearchState == AStarSearch<Planet>::SEARCH_STATE_FAILED ) 
    {
        astarsearch.EnsureMemoryFreed();
        return pl->planetID();
    }



    astarsearch.EnsureMemoryFreed();

    return pl->planetID();
}


//computes my growth rate in t steps, given current fleet movements
int MyBot::myPredictedGrowthRate(int t) {
    int gr(0);
    for(Planets::const_iterator pit = planets.begin();pit!=planets.end();++pit) {
        Planet* p = *pit;
        Planet pFut = predictions[p][t];
        if (pFut.owner()->isMe()) {
            gr += pFut.growthRate();
        }
    }
    return gr;
}


int MyBot::enemyPredictedGrowthRate(int t) {
    int gr(0);
    for(Planets::const_iterator pit = planets.begin();pit!=planets.end();++pit) {
        Planet* p = *pit;
        Planet pFut = predictions[p][t];
        if (pFut.owner()->isEnemy()) {
            gr += pFut.growthRate();
        }
    }
    return gr;
}

int MyBot::shipsAvailable(const vector<Planet>& predictions, int t) const{
    int available = predictions[0].shipsCount();
    int i(0);
    for(std::vector<Planet>::const_iterator pit = predictions.begin(); pit != predictions.end(); ++pit) {
        int sc = pit->shipsCount();
        //cerr << "sc: " << sc << endl;
        if (pit->owner()->isMe()) {
            available = min(available,sc);
        } else {
            available = min(available, -sc);
            //cerr << available << endl;
            break;
        }
        //cerr << available << endl;
        i++;
        if (i>t)break;
    }
    return available;
}
      

Planet* MyBot::coveredBy(Planet* pl, Planet* from) const{
    Planets closest = from->closestPlanets();
    for(Planets::const_iterator pit = closest.begin(); pit != closest.end(); ++pit){
        Planet* p = *pit;
        if (p == pl){
            return pl;
        }
        if(from->distance(p)+p->distance(pl) == from->distance(pl)){
            return p;
        }
    }
    return pl;
}
  



Planets MyBot::cluster(Planet* pl) const{
    Planets clusterPlanets;
    clusterPlanets.push_back(pl);
    Planet* p = nearestFrontierPlanet(pl);
    Planets closest = p->closestPlanets();
    for(Planets::iterator pit = closest.begin(); pit != closest.end(); ++pit){
        Planet* p2 = *pit;
        if (p2->frontierStatus){
            return clusterPlanets;
        }
        if (p2->owner()->isMe()){
            clusterPlanets.push_back(p2);
        }
    }
    return clusterPlanets;
}



bool MyBot::willHoldAtSomePoint(const vector<Planet>& preds) const{
    for(vector<Planet>::const_iterator p = preds.begin(); p != preds.end(); ++p){
        if(p->owner()->isMe()){
            return true;
        }
    }
    return false;
}

void MyBot::issueOrder(Order o, string reason){

    if(logging){
        cerr << "Sent " << o.shipsCount << " from " << o.sourcePlanet->planetID() << " to " << o.destinationPlanet->planetID() << " because of " << reason << endl;
    }
    if(o.isValid()){        
        game->issueOrder(o);
    } else {
        if(logging){
            cerr << "WARNING:  Order canceled, was not valid" << endl;
        }
    }
    
}




int MyBot::potential(Planet* pl){
    int pot(maxDistanceBetweenPlanets * pl->growthRate());
    Planets closest = pl->closestPlanets();
    for(Planets::iterator pit = closest.begin(); pit != closest.end(); ++pit){
        Planet* p = *pit;
        Planet pFut = predictions[p][lookahead];
        int diff = (maxDistanceBetweenPlanets - p->distance(pl)) * pl->growthRate();
        pot += diff;
    }
    return max(0,pot);
}
