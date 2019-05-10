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

namespace Algora {

class RandomDynamicDiGraphGenerator : public DynamicDiGraphProvider
{
public:
    RandomDynamicDiGraphGenerator()
        : DynamicDiGraphProvider(),
          iGraphSize(0ULL), iArcSize(0ULL), iArcProbability(0.0), multiArcs(true),
          numOperations(0ULL), propAddition(0U), propDeletion(0U), propAdvance(0U), multiplier(0U),
          numAdditions(0ULL), numDeletions(0ULL), numAdvances(0ULL), seed(0ULL), initialized(false) { }
    virtual ~RandomDynamicDiGraphGenerator() override { }

    void setInitialGraphSize(unsigned long long size) { iGraphSize = size; }
    void setInitialArcSize(unsigned long long size) { iArcSize = size; }
    void setInitialArcProbability(double p) { iArcProbability = p; }
    void allowMultiArcs(bool allow) { multiArcs = allow; }
    void setNumOperations(unsigned long long ops) { numOperations = ops; }
    void setArcAdditionProportion(unsigned int p) { propAddition = p; }
    void setArcRemovalProportion(unsigned int p) { propDeletion = p; }
    void setAdvanceTimeProportion(unsigned int p) { propAdvance = p; }
    void setMultiplier(unsigned int times) { multiplier = times; }
    void setSeed(unsigned int s) { seed = s; }

    unsigned long long getInitialGraphSize() const { return iGraphSize; }
    unsigned long long getInitialArcSize() const { return iArcSize; }
    bool multiArcsAllowed() const { return multiArcs; }
    unsigned long long getNumOperations() const { return numOperations; }
    unsigned int getArcAdditionProportion() const { return propAddition; }
    unsigned int getArcRemovalProportion() const { return propDeletion; }
    unsigned int getAdvanceTimeProportion() const { return propAdvance; }
    unsigned int getMultiplier() const { return multiplier; }
    unsigned long long getSeed() const { return seed; }

    unsigned long long numArcAdditions() const { return numAdditions; }
    unsigned long long numArcRemovals() const { return numDeletions; }
    unsigned long long numTimeAdvances() const { return numAdvances; }
    unsigned long long numDeltas() const { return numAdvances + 1ULL; }

    // DynamicDiGraphProvider interface
public:
    virtual bool isGraphAvailable() override { return true; }
    virtual bool provideDynamicDiGraph(DynamicDiGraph *dyGraph) override;
    virtual std::string getConfiguration() const override;
    virtual std::string getConfigurationAsJson(const std::string &indent) const override;
    virtual std::string getName() const noexcept override { return "Random Dynamic Digraph Generator"; }

private:
    unsigned long long iGraphSize;
    unsigned long long iArcSize;
    double iArcProbability;
    bool multiArcs;
    unsigned long long numOperations;
    unsigned int propAddition;
    unsigned int propDeletion;
    unsigned int propAdvance;
    unsigned int multiplier;

    unsigned long long numAdditions;
    unsigned long long numDeletions;
    unsigned long long numAdvances;

    unsigned long long seed;
    std::mt19937_64 gen;
    bool initialized;

    void init();
};

}

#endif // RANDOMDYNAMICDIGRAPHGENERATOR
