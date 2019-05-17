/**
 * Copyright (C) 2013 - 2018 : Kathrin Hanauer
 *
 * This file is part of Algora.
 *
 * Algora is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Algora is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Algora.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact information:
 *   http://algora.xaikal.org
 */

#include "randominstanceprovider.h"

#include "graph.dyn/dynamicdigraph.h"

#include <random>
#include <functional>
#include <cassert>

#include <iostream>
#include <iomanip>
#include <sstream>

namespace Algora {

struct RandomInstanceProvider::CheshireCat {
    unsigned int numInstances;
    unsigned int curInstance;
    unsigned long long seed;

    unsigned long long iGraphSize;
    unsigned long long iArcSize;
    double iArcProbability;
    bool multiArcs;
    unsigned long long numOperations;
    unsigned int propAddition;
    unsigned int propRemoval;
    unsigned int propQuery;
    unsigned int multiplier;

    DynamicDiGraph dynamicGraph;
    QueriesList queries;
    unsigned long long numArcAdditions;
    unsigned long long numArcRemovals;
    unsigned long long numQueries;
    double averageArcSize;
    unsigned long long finalArcSize;
    std::vector<unsigned long long> numVertices;
    std::vector<unsigned long long> numArcs;

    std::mt19937_64 gen;

    CheshireCat()
        : numInstances(1U), curInstance(0U), seed(0U),
          iGraphSize(0U), iArcSize(0U), iArcProbability(0.0),
          multiArcs(true), numOperations(10U),
          propAddition(0U), propRemoval(0U), propQuery(0U),
          multiplier(1U),
          numArcAdditions(0U), numArcRemovals(0U), numQueries(0U),
          averageArcSize(0.0) { }

    void init() {
        if (seed == 0U) {
            std::random_device rd;
            seed = rd();
        }
        std::cout << "Seed is " << seed << "." << std::endl;
        gen.seed(seed);
    }

    bool nextInstance() {
        if (curInstance < numInstances) {
            if (curInstance == 0) {
                init();
            }
            buildInstance();
            curInstance++;
            return true;
        }
        return false;
    }

    void buildInstance();
};

RandomInstanceProvider::RandomInstanceProvider()
    : grin(new CheshireCat)
{
}

RandomInstanceProvider::RandomInstanceProvider(const RandomInstanceProvider &other)
    : grin(new CheshireCat(*other.grin))
{
}

RandomInstanceProvider::~RandomInstanceProvider()
{
    delete grin;
}

void RandomInstanceProvider::setSeed(unsigned long long seed)
{
    grin->seed = seed;
}

void RandomInstanceProvider::setRepetitions(unsigned int r)
{
    grin->numInstances = r;
}

void RandomInstanceProvider::setGraphSize(unsigned long long n)
{
    grin->iGraphSize = n;
}

void RandomInstanceProvider::setInitialArcSize(unsigned long long m)
{
    grin->iArcSize = m;
}

void RandomInstanceProvider::setInitialArcProbability(double p)
{
    grin->iArcProbability = p;
}

void RandomInstanceProvider::allowMultiArcs(bool allow)
{
    grin->multiArcs = allow;
}

void RandomInstanceProvider::setNumOperations(unsigned long long ops)
{
    grin->numOperations = ops;
}

void RandomInstanceProvider::setArcAdditionProportion(unsigned int propAdd)
{
    grin->propAddition = propAdd;
}

void RandomInstanceProvider::setArcRemovalProportion(unsigned int propRemove)
{
    grin->propRemoval = propRemove;
}

void RandomInstanceProvider::setQueriesProportion(unsigned int propQuery)
{
    grin->propQuery = propQuery;
}

void RandomInstanceProvider::setMultiplier(unsigned int multiplier)
{
    grin->multiplier = multiplier;
}

unsigned long long RandomInstanceProvider::getGraphSize() const
{
    return grin->iGraphSize;
}

unsigned long long RandomInstanceProvider::getInitialArcSize() const
{
    return grin->iArcSize;
}

unsigned long long RandomInstanceProvider::numArcAdditions() const
{
    return grin->numArcAdditions;
}

unsigned long long RandomInstanceProvider::numArcRemovals() const
{
    return grin->numArcRemovals;
}

unsigned long long RandomInstanceProvider::numQueries() const
{
    return grin->numQueries;
}

unsigned long long RandomInstanceProvider::numDeltas() const
{
    return grin->queries.size() - 1;
}

std::string RandomInstanceProvider::getConfiguration() const
{
    std::stringstream ss;
    ss << std::setprecision(17);
    ss << "#Instances         : " << grin->numInstances << std::endl;
    ss << "#Vertices          : " << grin->iGraphSize << std::endl;
    ss << "#Arcs              : " << grin->iArcSize << std::endl;
    ss << "Arcs prob.         : " << grin->iArcProbability << std::endl;
    ss << "Multiarcs          : " << (grin->multiArcs ? "yes" : "no") << std::endl;
    ss << "#Operations        : " << grin->numOperations << std::endl;
    ss << "Prop. arc addition : " << grin->propAddition << std::endl;
    ss << "Prop. arc removal  : " << grin->propRemoval << std::endl;
    ss << "Prop. queries      : " << grin->propQuery << std::endl;
    ss << "Multiplier         : " << grin->multiplier<< std::endl;
    ss << "Seed               : " << grin->seed << std::endl;
    return ss.str();
}

std::string RandomInstanceProvider::getConfigurationAsJson(const std::string &indent) const
{
    std::stringstream ss;
    ss << std::setprecision(17);
    ss << indent <<   "\"num_graphs\": " << grin->numInstances << ",";
    ss << "\n" << indent << "\"vertices\": " << grin->iGraphSize << ",";
    ss << "\n" << indent << "\"arcs_init\": " << grin->iArcSize << ",";
    ss << "\n" << indent << "\"arcs_probability\": " << grin->iArcProbability << ",";
    ss << "\n" << indent << "\"multiarcs\": " << (grin->multiArcs ? "true" : "false") << ",";
    ss << "\n" << indent << "\"operations\": " << grin->numOperations << ",";
    ss << "\n" << indent << "\"prop_arc_addition\": " << grin->propAddition << ",";
    ss << "\n" << indent << "\"prop_arc_deletion\": " << grin->propRemoval << ",";
    ss << "\n" << indent << "\"prop_query\": " << grin->propQuery << ",";
    ss << "\n" << indent << "\"multiplier\": " << grin->multiplier << ",";
    ss << "\n" << indent << "\"seed\": " << grin->seed;

    return ss.str();
}

bool RandomInstanceProvider::nextInstance()
{
    return grin->nextInstance();
}

DynamicDiGraph &RandomInstanceProvider::getGraph() const
{
    return grin->dynamicGraph;
}

QueriesList &RandomInstanceProvider::getQueries() const
{
    return grin->queries;
}

unsigned long long RandomInstanceProvider::graphSize() const
{
    return grin->iGraphSize;
}

unsigned long long RandomInstanceProvider::initialGraphSize() const
{
    return grin->iGraphSize;
}

unsigned long long RandomInstanceProvider::initialArcSize() const
{
    return grin->iArcSize;
}

double RandomInstanceProvider::averageGraphSize() const
{
    return grin->iGraphSize;
}

double RandomInstanceProvider::averageArcSize() const
{
    return grin->averageArcSize;
}

unsigned long long RandomInstanceProvider::finalArcSize() const
{
    return grin->finalArcSize;
}

std::vector<unsigned long long> RandomInstanceProvider::numVertices() const
{
	return grin->numVertices;
}

std::vector<unsigned long long> RandomInstanceProvider::numArcs() const
{
	return grin->numArcs;
}

void RandomInstanceProvider::CheshireCat::buildInstance()
{
    std::uniform_int_distribution<unsigned long long> distVertex(0, iGraphSize - 1);
    std::uniform_int_distribution<unsigned long long> distSecVertex(0, iGraphSize - 2);
    auto randomVertex = std::bind(distVertex, std::ref(gen));
    auto randomSecondVertex = [&](unsigned long long first) {
        auto r = distSecVertex(gen);
        if (r >= first) {
            r++;
        }
        return r;
    };
    std::vector<std::pair<unsigned long long, unsigned long long>> arcs;
    unsigned long long timestamp = 0U;
    dynamicGraph.clear();
    queries.clear();
    numArcAdditions = 0U;
    numArcRemovals = 0U;
    numQueries = 0U;
    numVertices.clear();
    numArcs.clear();
    unsigned long long sumArcs = 0U;

    auto addRandomArc = [&](unsigned int mult) {
        for (auto i = 0U; i < mult; i++) {
            unsigned long long r1, r2;
            if (multiArcs) {
                r1 = randomVertex();
                r2 = randomSecondVertex(r1);
            } else {
                do {
                    r1 = randomVertex();
                    r2 = randomSecondVertex(r1);
                } while (dynamicGraph.hasArc(r1, r2));
            }

            //std::cout << "Adding arc " << r1 << " -> " << r2 << " at time " << timestamp << std::endl;
            dynamicGraph.addArc(r1, r2, timestamp);
            arcs.push_back(std::make_pair(r1, r2));
            numArcAdditions++;
            sumArcs += arcs.size();
            numArcs.back()++;
        }
        if (mult > 1U) {
            dynamicGraph.compact(mult);
        }
    };
    auto removeRandomArc = [&](unsigned int mult) {
        for (auto i = 0U; i < mult; i++) {
            if (arcs.empty()) {
                throw std::logic_error("List of arcs is empty.");
            }
            std::uniform_int_distribution<unsigned long long> dist(0, arcs.size() - 1);
            auto r = dist(gen);
            auto p = arcs[r];
            dynamicGraph.removeArc(p.first, p.second, timestamp);
            //std::cout << "Removing arc " << p.first << " -> " << p.second << " at time " << timestamp << std::endl;
            arcs[r] = arcs.back();
            arcs.pop_back();
            numArcRemovals++;
            sumArcs += arcs.size();
            numArcs.back()--;
        }
        if (mult > 1U) {
            dynamicGraph.compact(mult);
        }
    };
    auto randomQuery = [&](unsigned int mult) {
        for (auto i = 0U; i < mult; i++) {
            auto r = randomVertex();
            //std::cout << "Querying " << r << " at time " << timestamp << std::endl;
            queries.back().push_back(r);
            numQueries++;
        }
    };

    // add vertices
    for (auto i = 0ULL; i < iGraphSize; i++) {
        dynamicGraph.addVertex(timestamp);
    }
		numArcs.push_back(0U);

    // add initial arcs
    if (iArcProbability > 0) {
        iArcSize = 0U;
        std::uniform_real_distribution<> dis(0.0, 1.0);
        for (auto i = 0ULL; i < iGraphSize; i++) {
            for (auto j = 0ULL; j < iGraphSize; j++) {
                if (i == j) {
                    continue;
                }
                if (dis(gen) <= iArcProbability) {
                    dynamicGraph.addArc(i, j, timestamp);
                    arcs.push_back(std::make_pair(i, j));
                    iArcSize++;
                }
            }
        }
    } else  {
        for (auto i = 0ULL; i < iArcSize; i++) {
            addRandomArc(1U);
        }
    }
    // count only additions that are operations
    numArcAdditions = 0ULL;
    sumArcs = 0U;
    numVertices.push_back(iGraphSize);
    assert(numArcs.empty() || numArcs.back() == iArcSize);
    numArcs.clear();
    numArcs.push_back(iArcSize);

    auto propSum = propAddition + propRemoval + propQuery;
    auto thresholdRemoval = propAddition;
    auto thresholdQuery = thresholdRemoval + propRemoval;
    std::uniform_int_distribution<unsigned long long> distOps(0, propSum - 1);

    bool lastWasModification = true;
    bool first = true;
    queries.emplace_back();
    for (auto o = 0ULL; o < numOperations; o++) {
        auto r = distOps(gen);
        if (r < thresholdRemoval) {
            if (!lastWasModification || first) {
                timestamp++;
                queries.emplace_back();
                numVertices.push_back(numVertices.back());
                numArcs.push_back(numArcs.back());
            }
            addRandomArc(multiplier);
            lastWasModification = true;
            first = false;
        } else if (r < thresholdQuery) {
            if (!lastWasModification || first) {
                timestamp++;
                queries.emplace_back();
                numVertices.push_back(numVertices.back());
                numArcs.push_back(numArcs.back());
            }
            removeRandomArc(multiplier);
            first = false;
            lastWasModification = true;
        } else {
            randomQuery(multiplier);
            lastWasModification = false;
            first = false;
        }
    }
    if (lastWasModification) {
        // keep correct number of deltas as queries.size() - 1
        queries.emplace_back();
    }
    assert (numArcAdditions == dynamicGraph.countArcAdditions(1U, timestamp));
    assert (numArcRemovals == dynamicGraph.countArcRemovals(1U, timestamp));
    //std::cout << "#arc additions: " << numArcAdditions << ", #arc removals: " << numArcRemovals << ", arc sum: " << sumArcs << std::endl;
    averageArcSize = (iArcSize + sumArcs) / (1 + numArcAdditions + numArcRemovals);
    finalArcSize = arcs.size();
    assert(finalArcSize == numArcs.back());
}

} // namespace
