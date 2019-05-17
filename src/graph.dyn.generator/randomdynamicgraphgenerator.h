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

#ifndef RANDOMINSTANCEPROVIDER_H
#define RANDOMINSTANCEPROVIDER_H

#include "instanceprovider.h"

namespace Algora {

class RandomInstanceProvider : public InstanceProvider
{
public:
    explicit RandomInstanceProvider();
    RandomInstanceProvider(const RandomInstanceProvider &other);
    virtual ~RandomInstanceProvider();

    void setSeed(unsigned long long seed = 0U);
    void setRepetitions(unsigned r);
    void setGraphSize(unsigned long long n);
    void setInitialArcSize(unsigned long long m);
    void setInitialArcProbability(double p);
    void allowMultiArcs(bool allow);
    void setNumOperations(unsigned long long ops);
    void setArcAdditionProportion(unsigned int propAdd);
    void setArcRemovalProportion(unsigned int propRemove);
    void setQueriesProportion(unsigned int propQuery);
    void setMultiplier(unsigned int multiplier);

    unsigned long long getSeed() const;
    unsigned long long getNumOperations() const;
    unsigned long long getGraphSize() const;
    unsigned long long getInitialArcSize() const;
    unsigned long long getArcAdditionProportion() const;
    unsigned long long getArcRemovalProportion() const;
    unsigned long long getQueriesProportion() const;

    // InstanceProvider interface
public:
    virtual bool nextInstance() override;
    virtual DynamicDiGraph &getGraph() const override;
    virtual QueriesList &getQueries() const override;

    virtual unsigned long long graphSize() const override;
    virtual unsigned long long initialGraphSize() const override;
    virtual unsigned long long initialArcSize() const override;
    virtual double averageGraphSize() const override;
    virtual double averageArcSize() const override;
    virtual unsigned long long finalArcSize() const override;
    virtual unsigned long long numArcAdditions() const override;
    virtual unsigned long long numArcRemovals() const override;
    virtual unsigned long long numQueries() const override;
    virtual unsigned long long numDeltas() const override;
    virtual std::vector<unsigned long long> numVertices() const override;
    virtual std::vector<unsigned long long> numArcs() const override;

    virtual std::string getConfiguration() const override;
    virtual std::string getConfigurationAsJson(const std::string &indent) const override;
    virtual std::string getName() const noexcept override { return "Random Instance Provider"; }

private:
    struct CheshireCat;
    CheshireCat *grin;
};

}

#endif // RANDOMINSTANCEPROVIDER_H
