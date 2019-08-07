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

#include "dynamicdigraphstatistics.h"

#include "graph.dyn/dynamicdigraph.h"
#include "graph.incidencelist/incidencelistgraph.h"
#include <vector>
#include <algorithm>
#include <numeric>
#include <cassert>

namespace Algora {

template<typename C>
typename C::value_type medianOf(const C &container, const typename C::value_type &emptyValue = typename C::value_type()) {
    auto v(container);
    if (v.empty()) {
        return emptyValue;
    }
    auto half = v.size() / 2;
    std::nth_element(v.begin(), v.begin() + static_cast<typename C::difference_type>(half), v.end());
    return v[half];
}

void DynamicDiGraphStatistics::analyzeDynamicDiGraph(DynamicDiGraph *dyGraph)
{
    iGraphSize = 0U;
    maxGraphSize = 0U;
    minGraphSize = 0U;
    medGraphSize = 0U;
    avgGraphSize = 0.0;
    fGraphSize = 0U;
    std::vector<DiGraph::size_type> graphSizes;

    iArcSize = 0U;
    maxArcSize = 0U;
    minArcSize = 0U;
    medArcSize = 0U;
    avgArcSize = 0.0;
    fArcSize = 0U;
    std::vector<DiGraph::size_type> arcSizes;

    iDensity = 0.0;
    maxDensity = 0.0;
    minDensity = 0.0;
    medDensity = 0.0;
    avgDensity = 0.0;
    fDensity = 0.0;
    std::vector<double> densities;

    minArcAdditions = 0U;
    maxArcAdditions = 0U;
    medArcAdditions = 0U;
    sumArcAdditions = 0U;
    avgArcAdditions = 0.0;
    std::vector<DynamicDiGraph::size_type> arcAdditions;

    minArcRemovals = 0U;
    maxArcRemovals = 0U;
    medArcRemovals = 0U;
    sumArcRemovals = 0U;
    avgArcRemovals = 0.0;
    std::vector<DynamicDiGraph::size_type> arcRemovals;

    minTimeDelta = 0U;
    maxTimeDelta = 0U;
    medTimeDelta = 0U;
    sumTimeDelta = 0U;
    avgTimeDelta = 0.0;
    std::vector<DynamicDiGraph::DynamicTime> timeDeltas;

    dyGraph->resetToBigBang();
    auto graph = dyGraph->getDiGraph();

    bool next = dyGraph->applyNextDelta();
    iGraphSize = graph->getSize();
    iArcSize = graph->getNumArcs(true);
    iDensity = 1.0 * iArcSize / iGraphSize;
    auto tsOld = dyGraph->getCurrentTime();
    auto tsNew = tsOld;

    while (next) {
        auto n = graph->getSize();
        auto m = graph->getNumArcs(true);
        graphSizes.push_back(n);
        arcSizes.push_back(m);
        densities.push_back(1.0 * m / n);
        next = dyGraph->applyNextDelta();
        if (next) {
            tsOld = tsNew;
            tsNew = dyGraph->getCurrentTime();
            arcAdditions.push_back(dyGraph->countArcAdditions(tsNew, tsNew));
            arcRemovals.push_back(dyGraph->countArcRemovals(tsNew, tsNew));
            timeDeltas.push_back(tsNew - tsOld);
        }
    }

    assert(graphSizes.size() == arcSizes.size());
    assert(graphSizes.size() == densities.size());
    assert(arcSizes.size() == arcAdditions.size() + 1);
    assert(arcAdditions.size() == arcRemovals.size());
    assert(arcRemovals.size() == timeDeltas.size());

    auto p = std::minmax_element(std::begin(graphSizes), std::end(graphSizes));
    minGraphSize = *(p.first);
    maxGraphSize = *(p.second);
    medGraphSize = medianOf(graphSizes);
    avgGraphSize = (1.0 * std::accumulate(std::begin(graphSizes), std::end(graphSizes), 0ULL)) / graphSizes.size();
    fGraphSize = graphSizes.back();

    p = std::minmax_element(std::begin(arcSizes), std::end(arcSizes));
    minArcSize = *(p.first);
    maxArcSize = *(p.second);
    medArcSize = medianOf(arcSizes);
    avgArcSize = (1.0 * std::accumulate(std::begin(arcSizes), std::end(arcSizes), 0ULL)) / arcSizes.size();
    fArcSize = arcSizes.back();

    auto d = std::minmax_element(std::begin(densities), std::end(densities));
    minDensity = *(d.first);
    maxDensity = *(d.second);
    medDensity = medianOf(densities);
    avgDensity = (1.0 * std::accumulate(std::begin(densities), std::end(densities), 0ULL)) / densities.size();
    fDensity = densities.back();

    p = std::minmax_element(std::begin(arcAdditions), std::end(arcAdditions));
    minArcAdditions = *(p.first);
    maxArcAdditions = *(p.second);
    medArcAdditions = medianOf(arcAdditions);
    sumArcAdditions = std::accumulate(std::begin(arcAdditions), std::end(arcAdditions), 0ULL);
    avgArcAdditions = (1.0 * sumArcAdditions) / arcAdditions.size();

    p = std::minmax_element(std::begin(arcRemovals), std::end(arcRemovals));
    minArcRemovals = *(p.first);
    maxArcRemovals = *(p.second);
    medArcRemovals = medianOf(arcRemovals);
    sumArcRemovals = std::accumulate(std::begin(arcRemovals), std::end(arcRemovals), 0ULL);
    avgArcRemovals = (1.0 * sumArcRemovals) / arcRemovals.size();

    auto q = std::minmax_element(std::begin(timeDeltas), std::end(timeDeltas));
    minTimeDelta = *(q.first);
    maxTimeDelta = *(q.second);
    medTimeDelta = medianOf(timeDeltas);
    sumTimeDelta = std::accumulate(std::begin(timeDeltas), std::end(timeDeltas), 0ULL);
    avgTimeDelta = (1.0 * sumTimeDelta) / timeDeltas.size();
}


}
