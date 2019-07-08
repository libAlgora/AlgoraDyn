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
    void setGraphSize(DiGraph::size_type n);
    void setInitialArcSize(DiGraph::size_type m);
    void setInitialArcProbability(double p);
    void allowMultiArcs(bool allow);
    void setNumOperations(DynamicDiGraph::size_type ops);
    void setArcAdditionProportion(unsigned int propAdd);
    void setArcRemovalProportion(unsigned int propRemove);
    void setQueriesProportion(unsigned int propQuery);
    void setMultiplier(unsigned int multiplier);

    unsigned long long getSeed() const;
		DynamicDiGraph::size_type getNumOperations() const;
		DiGraph::size_type getGraphSize() const;
    DiGraph::size_type getInitialArcSize() const;
    unsigned int getArcAdditionProportion() const;
    unsigned int getArcRemovalProportion() const;
    unsigned int getQueriesProportion() const;

    // InstanceProvider interface
public:
    virtual bool nextInstance() override;
    virtual DynamicDiGraph &getGraph() const override;
    virtual QueriesList &getQueries() const override;

    virtual DiGraph::size_type graphSize() const override;
    virtual DiGraph::size_type initialGraphSize() const override;
    virtual DiGraph::size_type initialArcSize() const override;
    virtual double averageGraphSize() const override;
    virtual double averageArcSize() const override;
    virtual DiGraph::size_type finalArcSize() const override;
    virtual DynamicDiGraph::size_type numArcAdditions() const override;
    virtual DynamicDiGraph::size_type numArcRemovals() const override;
    virtual DynamicDiGraph::size_type numQueries() const override;
    virtual DynamicDiGraph::size_type numDeltas() const override;
    virtual std::vector<DiGraph::size_type> numVertices() const override;
    virtual std::vector<DiGraph::size_type> numArcs() const override;

    virtual std::string getConfiguration() const override;
    virtual std::string getConfigurationAsJson(const std::string &indent) const override;
    virtual std::string getName() const noexcept override { return "Random Instance Provider"; }

private:
    struct CheshireCat;
    CheshireCat *grin;
};

}

#endif // RANDOMINSTANCEPROVIDER_H
