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

#include "graph.dyn/dynamicdigraph.h"

namespace Algora {

class DynamicDiGraphGenerator
{
public:
    virtual bool nextInstance() override;
    virtual DynamicDiGraph &getGraph() const override;

    void setNumOperations(unsigned long long ops);
    void setArcAdditionProportion(unsigned int propAdd);
    void setArcRemovalProportion(unsigned int propRemove);
    void setAdvanceTimeProportion(unsigned int propTime);
    void setMultiplier(unsigned int multiplier);

    unsigned long long getInitialGraphSize() const;
    unsigned long long getInitialArcSize() const;
    unsigned long long getNumOperations() const;
    unsigned int getArcAdditionProportion() const;
    unsigned int getArcRemovalProportion() const;
    unsigned int getAdvanceTimeProportion() const;

    virtual unsigned long long numArcAdditions() const override;
    virtual unsigned long long numArcRemovals() const override;
    virtual unsigned long long numQueries() const override;
    virtual unsigned long long numDeltas() const override;

    virtual std::string getConfiguration() const override;
    virtual std::string getConfigurationAsJson(const std::string &indent) const override;
    virtual std::string getName() const noexcept override { return "Random Instance Provider"; }
};

}

#endif // DYNAMICDIGRAPHGENERATOR 
