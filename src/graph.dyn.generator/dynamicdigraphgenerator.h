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

#ifndef DYNAMICDIGRAPHGENERATOR_H
#define DYNAMICDIGRAPHGENERATOR_H

#include "pipe/dynamicdigraphprovider.h"

namespace Algora {

class DynamicDiGraphGenerator : public DynamicDiGraphProvider
{
public:
    DynamicDiGraphGenerator()
        : DynamicDiGraphProvider(),
          num_ops(0ULL), prop_addition(0U), prop_deletion(0U), prop_advance(0U), multiplier(0U),
          num_additions(0ULL), num_deletions(0ULL), num_advances(0ULL) { }
    virtual ~DynamicDiGraphGenerator();

    void setNumOperations(unsigned long long ops) { num_ops = ops; }
    void setArcAdditionProportion(unsigned int propAdd) { prop_addition = propAdd; }
    void setArcRemovalProportion(unsigned int propRemove) { prop_deletion = propRemove; }
    void setAdvanceTimeProportion(unsigned int propAdvance) { prop_advance = propAdvance; }
    void setMultiplier(unsigned int times) { multiplier = times; }

    unsigned long long getNumOperations() const { return num_ops; }
    unsigned int getArcAdditionProportion() const { return prop_addition; }
    unsigned int getArcRemovalProportion() const { return prop_deletion; }
    unsigned int getAdvanceTimeProportion() const { return prop_advance; }
    unsigned int getMultiplier() const { return multiplier; }

    unsigned long long numArcAdditions() const { return num_additions; }
    unsigned long long numArcRemovals() const { return num_deletions; }
    unsigned long long numTimeAdvances() const { return num_advances; }
    unsigned long long numDeltas() const { return num_advances + 1ULL; }

private:
    unsigned long long num_ops;
    unsigned int prop_addition;
    unsigned int prop_deletion;
    unsigned int prop_advance;
    unsigned int multiplier;

    unsigned long long num_additions;
    unsigned long long num_deletions;
    unsigned long long num_advances;
};

DynamicDiGraphGenerator::~DynamicDiGraphGenerator() {}

}

#endif // DYNAMICDIGRAPHGENERATOR 
