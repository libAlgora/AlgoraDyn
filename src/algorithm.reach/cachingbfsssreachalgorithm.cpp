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

#include "cachingbfsssreachalgorithm.h"
#include "algorithm/digraphalgorithmexception.h"
#include "property/fastpropertymap.h"
#include "algorithm.basic.traversal/breadthfirstsearch.h"

namespace Algora {

struct CachingBFSSSReachAlgorithm::CheshireCat {
    //FastPropertyMap<unsigned long long> levels;
    BreadthFirstSearch<FastPropertyMap,false> bfs;
    bool initialized;
    bool arcAdded;
    bool arcRemoved;

    CachingBFSSSReachAlgorithm *parent;
    Vertex *source;
    DiGraph *diGraph;

    CheshireCat()
        : initialized(false), arcAdded(false), arcRemoved(false) {
        bfs.computeValues(false);
    }

    void run() {
        bfs.setGraph(diGraph);
        bfs.setStartVertex(source);
        if (!bfs.prepare()) {
            throw DiGraphAlgorithmException(parent, "Could not prepare BFS algorithm.");
        }
        bfs.run();
        bfs.deliver();
        initialized = true;
        arcAdded = false;
        arcRemoved = false;

#ifdef COLLECT_PR_DATA
        parent->prReset();
        parent->prVerticesConsidered(diGraph->getSize());
        parent->prArcsConsidered(diGraph->getNumArcs());
#endif
    }

    bool query(const Vertex *t)
    {
        if (t == source || (initialized && !arcRemoved && bfs.vertexDiscovered(t))) {
            return true;
        } else if (initialized && !arcAdded && !bfs.vertexDiscovered(t)) {
            return false;
        }
        if (!initialized || arcAdded || arcRemoved) {
            run();
        }
        return bfs.vertexDiscovered(t);
    }
};

CachingBFSSSReachAlgorithm::CachingBFSSSReachAlgorithm()
    : DynamicSSReachAlgorithm(),
		grin(new CheshireCat)
{
		grin->parent = this;
}

CachingBFSSSReachAlgorithm::~CachingBFSSSReachAlgorithm()
{
	delete grin;
}

void CachingBFSSSReachAlgorithm::run()
{
	grin->run();
}

bool CachingBFSSSReachAlgorithm::query(const Vertex *t)
{
	return grin->query(t);
}

void CachingBFSSSReachAlgorithm::onDiGraphSet()
{
    DynamicSSReachAlgorithm::onDiGraphSet();
    //bfs.setGraph(diGraph);
    grin->initialized = false;
    grin->arcAdded = false;
    grin->arcRemoved = false;
    grin->diGraph = diGraph;
}

void CachingBFSSSReachAlgorithm::onDiGraphUnset()
{
    grin->bfs.unsetGraph();
    grin->initialized = false;
    grin->arcAdded = false;
    grin->arcRemoved = false;
    grin->diGraph = nullptr;
    DynamicSSReachAlgorithm::onDiGraphUnset();
}


void CachingBFSSSReachAlgorithm::onArcAdd(Arc *a)
{
    if (a->isLoop()) {
        return;
    }

    auto head = a->getHead();

    if (head == source || grin->arcAdded || !grin->initialized) {
        return;
    }

    auto tail = a->getTail();

    if (grin->bfs.vertexDiscovered(head) || !grin->bfs.vertexDiscovered(tail)) {
        return;
    }

    grin->arcAdded = true;
}

void CachingBFSSSReachAlgorithm::onArcRemove(Arc *a)
{
    if (!grin->initialized || grin->arcRemoved || a->isLoop()) {
        return;
    }

    auto head = a->getHead();

    if (head == source) {
        return;
    }

    if (!grin->bfs.vertexDiscovered(head)) {
        return;
    }

    grin->arcRemoved = true;
}

void CachingBFSSSReachAlgorithm::onSourceSet()
{
    DynamicSSReachAlgorithm::onSourceSet();
    grin->initialized = false;
    grin->arcAdded = false;
    grin->arcRemoved = false;
    grin->source = source;
}

} // end namespace
