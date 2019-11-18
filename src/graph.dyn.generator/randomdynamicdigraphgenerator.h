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

#ifndef RANDOMDYNAMICDIGRAPHGENERATOR_H
#define RANDOMDYNAMICDIGRAPHGENERATOR_H

#include "pipe/dynamicdigraphprovider.h"
#include <random>
#include "graph.dyn/dynamicdigraph.h"

namespace Algora {

class RandomDynamicDiGraphGenerator : public DynamicDiGraphProvider
{
public:
    RandomDynamicDiGraphGenerator()
        : DynamicDiGraphProvider(),
          iGraphSize(0U), iArcSize(0U),
          numOperations(0U),
          numAdditions(0U), numDeletions(0U), numAdvances(0U), seed(0U),
          iArcProbability(0.0),
          propAddition(0U), propDeletion(0U), propAdvance(0U), multiplier(0U),
          initialized(false), multiArcs(true) { }
    virtual ~RandomDynamicDiGraphGenerator() override { }

    void setInitialGraphSize(DiGraph::size_type size) { iGraphSize = size; }
    void setInitialArcSize(DiGraph::size_type size) { iArcSize = size; }
    void setInitialArcProbability(double p) { iArcProbability = p; }
    void allowMultiArcs(bool allow) { multiArcs = allow; }
    void setNumOperations(DynamicDiGraph::size_type ops) { numOperations = ops; }
    void setArcAdditionProportion(unsigned int p) { propAddition = p; }
    void setArcRemovalProportion(unsigned int p) { propDeletion = p; }
    void setAdvanceTimeProportion(unsigned int p) { propAdvance = p; }
    void setMultiplier(unsigned int times) { multiplier = times; }
    void setSeed(unsigned long long s) { seed = s; }

    DiGraph::size_type getInitialGraphSize() const { return iGraphSize; }
    DiGraph::size_type getInitialArcSize() const { return iArcSize; }
    bool multiArcsAllowed() const { return multiArcs; }
    DynamicDiGraph::size_type getNumOperations() const { return numOperations; }
    unsigned int getArcAdditionProportion() const { return propAddition; }
    unsigned int getArcRemovalProportion() const { return propDeletion; }
    unsigned int getAdvanceTimeProportion() const { return propAdvance; }
    unsigned int getMultiplier() const { return multiplier; }
    unsigned long long getSeed() const { return seed; }

    DynamicDiGraph::size_type numArcAdditions() const { return numAdditions; }
    DynamicDiGraph::size_type numArcRemovals() const { return numDeletions; }
    DynamicDiGraph::size_type numTimeAdvances() const { return numAdvances; }
    DynamicDiGraph::size_type numDeltas() const { return numAdvances + 1U; }

    // DynamicDiGraphProvider interface
public:
    virtual bool isGraphAvailable() override { return true; }
    virtual bool provideDynamicDiGraph(DynamicDiGraph *dyGraph) override;
    virtual std::string getConfiguration() const override;
    virtual std::ostream &toJson(std::ostream &out, const std::string &newline) const override;
    virtual std::string getName() const noexcept override { return "Random Dynamic Digraph Generator"; }

private:
    DiGraph::size_type iGraphSize;
    DiGraph::size_type iArcSize;
    DynamicDiGraph::size_type numOperations;

    DynamicDiGraph::size_type numAdditions;
    DynamicDiGraph::size_type numDeletions;
    DynamicDiGraph::size_type numAdvances;

    unsigned long long seed;
    std::mt19937_64 gen;

    double iArcProbability;
    unsigned int propAddition;
    unsigned int propDeletion;
    unsigned int propAdvance;
    unsigned int multiplier;

    bool initialized;
    bool multiArcs;

    void init();
};

}

#endif // RANDOMDYNAMICDIGRAPHGENERATOR
