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
    std::nth_element(v.begin(), v.begin()+half, v.end());
    return v[half];
}

void DynamicDiGraphStatistics::analyzeDynamicDiGraph(DynamicDiGraph *dyGraph)
{
    iGraphSize = 0ULL;
    maxGraphSize = 0ULL;
    minGraphSize = 0ULL;
    medGraphSize = 0ULL;
    avgGraphSize = 0.0;
    fGraphSize = 0ULL;
    std::vector<unsigned long long> graphSizes;

    iArcSize = 0ULL;
    maxArcSize = 0ULL;
    minArcSize = 0ULL;
    medArcSize = 0ULL;
    avgArcSize = 0.0;
    fArcSize = 0ULL;
    std::vector<unsigned long long> arcSizes;

    minArcAdditions = 0ULL;
    maxArcAdditions = 0ULL;
    medArcAdditions = 0ULL;
    sumArcAdditions = 0ULL;
    avgArcAdditions = 0.0;
    std::vector<unsigned long long> arcAdditions;

    minArcRemovals = 0ULL;
    maxArcRemovals = 0ULL;
    medArcRemovals = 0ULL;
    sumArcRemovals = 0ULL;
    avgArcRemovals = 0.0;
    std::vector<unsigned long long> arcRemovals;

    minTimeDelta = 0ULL;
    maxTimeDelta = 0ULL;
    medTimeDelta = 0ULL;
    sumTimeDelta = 0ULL;
    avgTimeDelta = 0.0;
    std::vector<unsigned long long> timeDeltas;

    dyGraph->resetToBigBang();
    auto graph = dyGraph->getDiGraph();

    bool next = dyGraph->applyNextDelta();
    iGraphSize = graph->getSize();
    iArcSize = graph->getNumArcs(false);
    auto tsOld = dyGraph->getCurrentTime();
    auto tsNew = tsOld;

    while (next) {
        graphSizes.push_back(graph->getSize());
        arcSizes.push_back(graph->getNumArcs(false));
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
    assert(arcSizes.size() == arcAdditions.size() + 1);
    assert(arcAdditions.size() == arcRemovals.size());
    assert(arcRemovals.size() == timeDeltas.size());

    auto p = std::minmax_element(std::begin(graphSizes), std::end(graphSizes));
    minGraphSize = *(p.first);
    maxGraphSize = *(p.second);
    medGraphSize = medianOf(graphSizes);
    avgGraphSize = (1.0 * std::accumulate(std::begin(graphSizes), std::end(graphSizes), 0ULL)) / graphSizes.size();

    p = std::minmax_element(std::begin(arcSizes), std::end(arcSizes));
    minArcSize = *(p.first);
    maxArcSize = *(p.second);
    medArcSize = medianOf(arcSizes);
    avgArcSize = (1.0 * std::accumulate(std::begin(arcSizes), std::end(arcSizes), 0ULL)) / arcSizes.size();

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

    p = std::minmax_element(std::begin(timeDeltas), std::end(timeDeltas));
    minTimeDelta = *(p.first);
    maxTimeDelta = *(p.second);
    medTimeDelta = medianOf(timeDeltas);
    sumTimeDelta = std::accumulate(std::begin(timeDeltas), std::end(timeDeltas), 0ULL);
    avgTimeDelta = (1.0 * sumTimeDelta) / timeDeltas.size();
}


}
