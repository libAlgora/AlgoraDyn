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

#include "staticbfsssreachalgorithm.h"
#include "property/fastpropertymap.h"

#include "algorithm.basic.traversal/breadthfirstsearch.h"

namespace Algora {

StaticBFSSSReachAlgorithm::StaticBFSSSReachAlgorithm()
    : DynamicSSReachAlgorithm()
{

}

StaticBFSSSReachAlgorithm::~StaticBFSSSReachAlgorithm()
{

}

void StaticBFSSSReachAlgorithm::run()
{

}

bool StaticBFSSSReachAlgorithm::query(const Vertex *t)
{
    if (t == source) {
        return true;
    }
    BreadthFirstSearch<FastPropertyMap> bfs(false);
    bfs.setStartVertex(source);
    bool reachable = false;
#ifdef COLLECT_PR_DATA
    bfs.onArcDiscover([&](const Arc *) {
        prArcConsidered();
        return true;
    });
    bfs.onVertexDiscover([&](const Vertex *) {
        prVertexConsidered();
        return true;
    });
#endif
    bfs.setArcStopCondition([&](const Arc *a) {
        if (a->getHead() == t) {
            reachable = true;
        }
        return reachable;
    });
    runAlgorithm(bfs, diGraph);
    return reachable;
}

}
