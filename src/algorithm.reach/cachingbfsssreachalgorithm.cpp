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

#include "cachingbfsssreachalgorithm.h"
#include "algorithm/digraphalgorithmexception.h"

namespace Algora {

CachingBFSSSReachAlgorithm::CachingBFSSSReachAlgorithm()
    : DynamicSSReachAlgorithm(), initialized(false),
      arcAdded(false), arcRemoved(false)
{
    levels.setDefaultValue(bfs.INF);
    bfs.useModifiableProperty(&levels);
    bfs.orderAsValues(false);
}

CachingBFSSSReachAlgorithm::~CachingBFSSSReachAlgorithm()
{

}

void CachingBFSSSReachAlgorithm::run()
{
    levels.resetAll();
    bfs.setStartVertex(source);
    if (!bfs.prepare()) {
       throw DiGraphAlgorithmException(this, "Could not prepare BFS algorithm.");
    }
    bfs.run();
    bfs.deliver();
    initialized = true;
    arcAdded = false;
    arcRemoved = false;

#ifdef COLLECT_PR_DATA
    prReset();
    prVerticesConsidered(diGraph->getSize());
    prArcsConsidered(diGraph->getNumArcs());
#endif
}

bool CachingBFSSSReachAlgorithm::query(const Vertex *t)
{
    if (t == source || (initialized && !arcRemoved && levels(t) != bfs.INF)) {
        return true;
    } else if (initialized && !arcAdded && levels(t) == bfs.INF) {
        return false;
    }
    if (!initialized || arcAdded || arcRemoved) {
        if (!prepare()) {
            throw DiGraphAlgorithmException(this, "Could not prepare myself.");
        }
        run();
    }
    return levels(t) != bfs.INF;
}

void CachingBFSSSReachAlgorithm::onDiGraphSet()
{
    DynamicSSReachAlgorithm::onDiGraphSet();
    bfs.setGraph(diGraph);
    initialized = false;
    arcAdded = false;
    arcRemoved = false;
}

void CachingBFSSSReachAlgorithm::onDiGraphUnset()
{
    bfs.unsetGraph();
    initialized = false;
    arcAdded = false;
    arcRemoved = false;
    DynamicSSReachAlgorithm::onDiGraphUnset();
}

void CachingBFSSSReachAlgorithm::onVertexAdd(Vertex *v)
{
    levels[v] = bfs.INF;
}

void CachingBFSSSReachAlgorithm::onVertexRemove(Vertex *v)
{
    levels.resetToDefault(v);
}

void CachingBFSSSReachAlgorithm::onArcAdd(Arc *a)
{
    if (!initialized || arcAdded || a->isLoop()) {
        return;
    }

    auto head = a->getHead();

    if (head == source ) {
        return;
    }

    auto tail = a->getTail();

    if (levels(head) != bfs.INF || levels(tail) == bfs.INF) {
        return;
    }

    arcAdded = true;
}

void CachingBFSSSReachAlgorithm::onArcRemove(Arc *a)
{
    if (!initialized || arcRemoved || a->isLoop()) {
        return;
    }

    auto head = a->getHead();

    if (head == source) {
        return;
    }

    if (levels(head) == bfs.INF) {
        return;
    }

    arcRemoved = true;
}

void CachingBFSSSReachAlgorithm::onSourceSet()
{
    DynamicSSReachAlgorithm::onSourceSet();
    initialized = false;
    arcAdded = false;
    arcRemoved = false;
}

} // end namespace
