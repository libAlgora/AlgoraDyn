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
    BreadthFirstSearch<FastPropertyMap,false> bfs;
    bool initialized;
    bool arcAdded;
    bool arcRemoved;
    FastPropertyMap<Arc*> treeArc;

    CachingBFSSSReachAlgorithm *parent;
    Vertex *source;
    DiGraph *diGraph;

    CheshireCat()
        : initialized(false), arcAdded(false), arcRemoved(false) {
        bfs.computeValues(false);
        treeArc.setDefaultValue(nullptr);
    }

    void run() {
        treeArc.resetAll();
        bfs.setStartVertex(source);
        bfs.onTreeArcDiscover([this](const Arc *a) {
            treeArc[a->getHead()] = const_cast<Arc*>(a);
            return true;
        });
        runAlgorithm(bfs, diGraph);
        initialized = true;
        arcAdded = false;
        arcRemoved = false;

#ifdef COLLECT_PR_DATA
        parent->prReset();
        parent->prVerticesConsidered(diGraph->getSize());
        parent->prArcsConsidered(diGraph->getNumArcs(true));
#endif
    }

    bool query(const Vertex *t)
    {
        if (t == source) {
            return true;
        } else if (diGraph->isSink(source) || diGraph->isSource(t)) {
            return false;
        } else if (initialized && !arcRemoved && bfs.vertexDiscovered(t)) {
            return true;
        } else if (initialized && !arcAdded && !bfs.vertexDiscovered(t)) {
            return false;
        }
        if (!initialized || arcAdded || arcRemoved) {
            run();
        }
        return bfs.vertexDiscovered(t);
    }

    void constructPath(const Vertex *t, std::vector<Arc *> &path) {
        while (t != source) {
            auto *a = treeArc(t);
            path.push_back(a);
            t = a->getTail();
        }
        assert(!path.empty());

        std::reverse(path.begin(), path.end());
    }

    void queryPath(const Vertex *t, std::vector<Arc *> &path) {
        if (t == source || diGraph->isSink(source) || diGraph->isSource(t)
             || (initialized && !arcAdded && !bfs.vertexDiscovered(t))) {
            return;
        } else if (initialized && !arcRemoved && bfs.vertexDiscovered(t)) {
            constructPath(t, path);
            return;
        }
        if (!initialized || arcAdded || arcRemoved) {
            run();
        }
        if (bfs.vertexDiscovered(t)) {
            constructPath(t, path);
        }
    }
};

CachingBFSSSReachAlgorithm::CachingBFSSSReachAlgorithm()
    : DynamicSingleSourceReachabilityAlgorithm(),
		grin(new CheshireCat)
{
    grin->parent = this;
    registerEvents(false, false, true, true);
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

std::vector<Arc *> CachingBFSSSReachAlgorithm::queryPath(const Vertex *t)
{
    std::vector<Arc*> path;
    grin->queryPath(t, path);
    assert (!query(t) || !path.empty() || t == source);
    return path;
}

void CachingBFSSSReachAlgorithm::onDiGraphSet()
{
    DynamicSingleSourceReachabilityAlgorithm::onDiGraphSet();
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
    DynamicSingleSourceReachabilityAlgorithm::onDiGraphUnset();
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

    if (a != grin->treeArc(head)) {
        return;
    }

    grin->arcRemoved = true;
}

void CachingBFSSSReachAlgorithm::onSourceSet()
{
    DynamicSingleSourceReachabilityAlgorithm::onSourceSet();
    grin->initialized = false;
    grin->arcAdded = false;
    grin->arcRemoved = false;
    grin->source = source;
}

} // end namespace
