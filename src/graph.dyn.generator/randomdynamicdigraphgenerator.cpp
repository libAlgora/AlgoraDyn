/**
 * Copyright (C) 2013 - 2019 : Kathrin Hanauer
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

#include "randomdynamicdigraphgenerator.h"

#include "graph.dyn/dynamicdigraph.h"

#include <functional>
#include <cassert>

#include <iostream>
#include <iomanip>
#include <sstream>

namespace Algora {

bool RandomDynamicDiGraphGenerator::provideDynamicDiGraph(DynamicDiGraph *dyGraph)
{
    init();

    std::uniform_int_distribution<DiGraph::size_type> distVertex(0, iGraphSize - 1);
    std::uniform_int_distribution<DiGraph::size_type> distSecVertex(0, iGraphSize - 2);
    auto randomVertex = std::bind(distVertex, std::ref(gen));
    auto randomSecondVertex = [&](DiGraph::size_type first) {
        auto r = distSecVertex(gen);
        if (r >= first) {
            r++;
        }
        return r;
    };

    std::vector<std::pair<DiGraph::size_type, DiGraph::size_type>> arcs;
		DynamicDiGraph::DynamicTime timestamp = 0U;
    dyGraph->clear();
    numAdditions = 0U;
    numDeletions = 0U;
    numAdvances = 0U;

    auto addRandomArc = [&](unsigned int mult) {
        for (auto i = 0U; i < mult; i++) {
					DiGraph::size_type r1, r2;
            if (multiArcs) {
                r1 = randomVertex();
                r2 = randomSecondVertex(r1);
            } else {
                do {
                    r1 = randomVertex();
                    r2 = randomSecondVertex(r1);
                } while (dyGraph->hasArc(r1, r2));
            }

            //std::cout << "Adding arc " << r1 << " -> " << r2 << " at time " << timestamp << std::endl;
            dyGraph->addArc(r1, r2, timestamp);
            arcs.push_back(std::make_pair(r1, r2));
            numAdditions++;
        }
        if (mult > 1U) {
            dyGraph->compact(mult);
        }
    };
    auto removeRandomArc = [&](unsigned int mult) {
        for (auto i = 0U; i < mult; i++) {
            if (arcs.empty()) {
                throw std::logic_error("List of arcs is empty.");
            }
            std::uniform_int_distribution<DiGraph::size_type> dist(0, arcs.size() - 1);
            auto r = dist(gen);
            auto p = arcs[r];
            dyGraph->removeArc(p.first, p.second, timestamp);
            //std::cout << "Removing arc " << p.first << " -> " << p.second << " at time " << timestamp << std::endl;
            arcs[r] = arcs.back();
            arcs.pop_back();
            numDeletions++;
        }
        if (mult > 1U) {
            dyGraph->compact(mult);
        }
    };

    // add vertices
    for (DiGraph::size_type i = 0U; i < iGraphSize; i++) {
        dyGraph->addVertex(timestamp);
    }

    // add initial arcs
    if (iArcProbability > 0.0) {
        iArcSize = 0U;
        std::uniform_real_distribution<> dis(0.0, 1.0);
        for (DiGraph::size_type i = 0U; i < iGraphSize; i++) {
            for (DiGraph::size_type j = 0U; j < iGraphSize; j++) {
                if (i == j) {
                    continue;
                }
                if (dis(gen) <= iArcProbability) {
                    dyGraph->addArc(i, j, timestamp);
                    arcs.push_back(std::make_pair(i, j));
                    iArcSize++;
                }
            }
        }
    } else  {
        for (DiGraph::size_type i = 0U; i < iArcSize; i++) {
            addRandomArc(1U);
        }
    }

    // count only additions that are operations -> reset counter
    numAdditions = 0U;

    auto propSum = propAddition + propDeletion + propAdvance;
    auto thresholdRemoval = propAddition;
    auto thresholdAdvance = thresholdRemoval + propDeletion;
    std::uniform_int_distribution<unsigned int> distOps(0, propSum - 1);

    timestamp++;
    for (DynamicDiGraph::size_type o = 0U; o < numOperations; o++) {
        auto r = distOps(gen);
        if (r < thresholdRemoval) {
            addRandomArc(multiplier);
        } else if (r < thresholdAdvance) {
            removeRandomArc(multiplier);
        } else {
            timestamp += multiplier;
            numAdvances++;
        }
    }
    if (dyGraph->getMaxTime() < timestamp) {
        dyGraph->noop(timestamp);
    }

    assert(numAdditions == dyGraph->countArcAdditions(1U, timestamp));
    assert(numDeletions == dyGraph->countArcRemovals(1U, timestamp));
    return true;
}

std::string RandomDynamicDiGraphGenerator::getConfiguration() const
{
    std::stringstream ss;
    ss << std::setprecision(17);
    ss << "#Vertices          : " << iGraphSize << std::endl;
    ss << "#Arcs              : " << iArcSize << std::endl;
    ss << "Arcs prob.         : " << iArcProbability << std::endl;
    ss << "Multiarcs          : " << (multiArcs ? "yes" : "no") << std::endl;
    ss << "#Operations        : " << numOperations << std::endl;
    ss << "Prop. arc addition : " << propAddition << std::endl;
    ss << "Prop. arc removal  : " << propDeletion << std::endl;
    ss << "Prop. time advance : " << propAdvance << std::endl;
    ss << "Multiplier         : " << multiplier<< std::endl;
    ss << "Seed               : " << seed << std::endl;
    return ss.str();
}

std::string RandomDynamicDiGraphGenerator::getConfigurationAsJson(const std::string &indent) const
{
    std::stringstream ss;
    ss << std::setprecision(17);
    ss << indent << "\"vertices\": " << iGraphSize << ",";
    ss << "\n" << indent << "\"arcs_init\": " << iArcSize << ",";
    ss << "\n" << indent << "\"arcs_probability\": " << iArcProbability << ",";
    ss << "\n" << indent << "\"multiarcs\": " << (multiArcs ? "true" : "false") << ",";
    ss << "\n" << indent << "\"operations\": " << numOperations << ",";
    ss << "\n" << indent << "\"prop_arc_addition\": " << propAddition << ",";
    ss << "\n" << indent << "\"prop_arc_deletion\": " << propDeletion << ",";
    ss << "\n" << indent << "\"prop_time_advance\": " << propAdvance << ",";
    ss << "\n" << indent << "\"multiplier\": " << multiplier << ",";
    ss << "\n" << indent << "\"seed\": " << seed;

    return ss.str();
}

void RandomDynamicDiGraphGenerator::init()
{
    if (initialized) {
        return;
    }

    if (seed == 0U) {
        std::random_device rd;
        seed = rd();
    }
    //        std::cout << "Seed is " << seed << "." << std::endl;
    gen.seed(seed);
    initialized = true;
}

}
