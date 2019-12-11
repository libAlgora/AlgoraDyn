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

#include <cassert>
#include <cmath>

#include "staticbfsssreachalgorithm.h"
#include "property/fastpropertymap.h"
#include "algorithm.basic/finddipathalgorithm.h"

#include "algorithm.basic.traversal/breadthfirstsearch.h"

namespace Algora {

template<bool reverseArcDirection>
StaticBFSSSReachAlgorithm<reverseArcDirection>::StaticBFSSSReachAlgorithm(bool twoWayBFS)
    : DynamicSingleSourceReachabilityAlgorithm(), twoWayBFS(twoWayBFS), bfsStepSize(5UL)
{
    registerEvents(false, false, false, false);
    fpa.setConstructPaths(false, false);
    fpa.useTwoWaySearch(twoWayBFS);
}

template<bool reverseArcDirection>
void StaticBFSSSReachAlgorithm<reverseArcDirection>::run()
{

}

template<bool reverseArcDirection>
void StaticBFSSSReachAlgorithm<reverseArcDirection>::onDiGraphSet()
{
    bfsStepSize = static_cast<DiGraph::size_type>(
                ceil(diGraph->getNumArcs(true) / diGraph->getSize()));
    if (bfsStepSize < 5) {
        bfsStepSize = 5;
    }
    fpa.setGraph(diGraph);
    fpa.setTwoWayStepSize(bfsStepSize);
}

template<bool reverseArcDirection>
bool StaticBFSSSReachAlgorithm<reverseArcDirection>::query(const Vertex *t)
{
    fpa.setConstructPaths(false, false);
    if (reverseArcDirection) {
        fpa.setSourceAndTarget(const_cast<Vertex*>(t), source);
    } else {
        fpa.setSourceAndTarget(source, const_cast<Vertex*>(t));
    }
    // fpa.prepare() omitted for performance reasons
    fpa.run();
    return fpa.deliver();
}

template<bool reverseArcDirection>
std::vector<Arc *> StaticBFSSSReachAlgorithm<reverseArcDirection>::queryPath(const Vertex *t)
{
    fpa.setConstructPaths(false, true);
    if (reverseArcDirection) {
        fpa.setSourceAndTarget(const_cast<Vertex*>(t), source);
    } else {
        fpa.setSourceAndTarget(source, const_cast<Vertex*>(t));
    }
    // fpa.prepare() omitted for performance reasons
    fpa.run();
    if (fpa.deliver()) {
        return fpa.deliverArcsOnPath();
    }
    return std::vector<Arc*>();
}

template class StaticBFSSSReachAlgorithm<false>;
template class StaticBFSSSReachAlgorithm<true>;
}
