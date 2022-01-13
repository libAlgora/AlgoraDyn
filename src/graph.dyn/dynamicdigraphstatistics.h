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

#ifndef DYNAMICDIGRAPHSTATISTICS_H
#define DYNAMICDIGRAPHSTATISTICS_H

#include "graph/digraph.h"
#include "graph.dyn/dynamicdigraph.h"

namespace Algora {

class DynamicDiGraph;

class DynamicDiGraphStatistics
{
public:
    DynamicDiGraphStatistics() {}

    void analyzeDynamicDiGraph(DynamicDiGraph *dyGraph);
    void clear();

    DiGraph::size_type initialGraphSize() const { return iGraphSize; }
    DiGraph::size_type maximumGraphSize() const { return maxGraphSize; }
    DiGraph::size_type minimumGraphSize() const { return minGraphSize; }
    DiGraph::size_type medianGraphSize() const { return medGraphSize; }
    DiGraph::size_type finalGraphSize() const { return fGraphSize; }
    double averageGraphSize() const { return avgGraphSize; }
    const std::vector<DiGraph::size_type> &getGraphSizes() const { return graphSizes; }

    DiGraph::size_type initialArcSize() const { return iArcSize; }
    DiGraph::size_type maximumArcSize() const { return maxArcSize; }
    DiGraph::size_type minimumArcSize() const { return minArcSize; }
    DiGraph::size_type medianArcSize() const { return medArcSize; }
    DiGraph::size_type finalArcSize() const { return fArcSize; }
    double averageArcSize() const { return avgArcSize; }
    const std::vector<DiGraph::size_type> &getArcSizes() const { return arcSizes; }

    double initialDensity() const { return iDensity; }
    double maximumDensity() const { return maxDensity; }
    double minimumDensity() const { return minDensity; }
    double medianDensity() const { return medDensity; }
    double finalDensity() const { return fDensity; }
    double averageDensity() const { return avgDensity; }
    const std::vector<double> &getDensities() const { return densities; }

    DynamicDiGraph::size_type minimumArcAdditions() const { return minArcAdditions; }
    DynamicDiGraph::size_type maximumArcAdditions() const { return maxArcAdditions; }
    DynamicDiGraph::size_type medianArcAdditions() const { return medArcAdditions; }
    DynamicDiGraph::size_type totalArcAdditions() const { return sumArcAdditions; }
    double averageArcAdditions() const { return avgArcAdditions; }
    const std::vector<DynamicDiGraph::size_type> &getArcAdditions() const { return arcAdditions; }

    DynamicDiGraph::size_type minimumArcRemovals() const { return minArcRemovals; }
    DynamicDiGraph::size_type maximumArcsRemovals() const { return maxArcRemovals; }
    DynamicDiGraph::size_type medianArcRemovals() const { return medArcRemovals; }
    DynamicDiGraph::size_type totalArcRemovals() const { return sumArcRemovals; }
    double averageArcRemovals() const { return avgArcRemovals; }
    const std::vector<DynamicDiGraph::size_type> &getArcRemovals() const { return arcRemovals; }

    DynamicDiGraph::DynamicTime minimumTimeDelta() const { return minTimeDelta; }
    DynamicDiGraph::DynamicTime maximumTimeDelta() const { return maxTimeDelta; }
    DynamicDiGraph::DynamicTime medianTimeDelta() const { return medTimeDelta; }
    DynamicDiGraph::DynamicTime totalTimeDelta() const { return sumTimeDelta; }
    double averageTimeDelta() const { return avgTimeDelta; }
    const std::vector<DynamicDiGraph::DynamicTime> &getTimeDeltas() const { return timeDeltas; }

    const std::vector<DynamicDiGraph::DynamicTime> &getTimestamps() const { return timestamps; }

    DiGraph::size_type maximumArcAge() const { return maxArcAge; }
    DiGraph::size_type minimumArcAge() const { return minArcAge; }
    DiGraph::size_type medianArcAge() const { return medArcAge; }
    DiGraph::size_type lowerQuartileArcAge() const { return lowQuartileArcAge; }
    DiGraph::size_type upperQuartileArcAge() const { return upQuartileArcAge; }
    double averageArcAge() const { return avgArcAge; }
    const std::vector<DiGraph::size_type> &getArcAges() const { return arcAges; }

private:
    std::vector<DiGraph::size_type> graphSizes;
    DiGraph::size_type iGraphSize;
    DiGraph::size_type maxGraphSize;
    DiGraph::size_type minGraphSize;
    DiGraph::size_type medGraphSize;
    DiGraph::size_type fGraphSize;
    double avgGraphSize;

    std::vector<DiGraph::size_type> arcSizes;
    DiGraph::size_type iArcSize;
    DiGraph::size_type maxArcSize;
    DiGraph::size_type minArcSize;
    DiGraph::size_type medArcSize;
    DiGraph::size_type fArcSize;
    double avgArcSize;

    std::vector<double> densities;
    double iDensity;
    double maxDensity;
    double minDensity;
    double medDensity;
    double fDensity;
    double avgDensity;

    std::vector<DynamicDiGraph::size_type> arcAdditions;
    DynamicDiGraph::size_type minArcAdditions;
    DynamicDiGraph::size_type maxArcAdditions;
    DynamicDiGraph::size_type medArcAdditions;
    DynamicDiGraph::size_type sumArcAdditions;
    double avgArcAdditions;

    std::vector<DynamicDiGraph::size_type> arcRemovals;
    DynamicDiGraph::size_type minArcRemovals;
    DynamicDiGraph::size_type maxArcRemovals;
    DynamicDiGraph::size_type medArcRemovals;
    DynamicDiGraph::size_type sumArcRemovals;
    double avgArcRemovals;

    std::vector<DynamicDiGraph::DynamicTime> timeDeltas;
    DynamicDiGraph::DynamicTime minTimeDelta;
    DynamicDiGraph::DynamicTime maxTimeDelta;
    DynamicDiGraph::DynamicTime medTimeDelta;
    DynamicDiGraph::DynamicTime sumTimeDelta;
    double avgTimeDelta;

    std::vector<DynamicDiGraph::DynamicTime> timestamps;

    std::vector<DiGraph::size_type> arcAges;
    DiGraph::size_type maxArcAge;
    DiGraph::size_type minArcAge;
    DiGraph::size_type medArcAge;
    DiGraph::size_type lowQuartileArcAge;
    DiGraph::size_type upQuartileArcAge;
    double avgArcAge;
};

}

#endif // DYNAMICDIGRAPHSTATISTICS_H
