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

#include "cachingdfsssreachalgorithm.h"
#include "algorithm/digraphalgorithmexception.h"

namespace Algora {

struct CachingDFSSSReachAlgorithm::CheshireCat {

    DepthFirstSearch<FastPropertyMap> dfs;
    bool initialized;
    bool arcAdded;
    bool arcRemoved;
    FastPropertyMap<Arc*> treeArc;

    CachingDFSSSReachAlgorithm *parent;
    DiGraph *diGraph;
    Vertex *source;

    CheshireCat()
        : initialized(false), arcAdded(false), arcRemoved(false) {
        dfs.computeValues(false);
        treeArc.setDefaultValue(nullptr);
    }

    void run() {
        treeArc.resetAll();
        dfs.setStartVertex(source);
        dfs.onTreeArcDiscover([this](const Arc *a) {
            treeArc[a->getHead()] = const_cast<Arc*>(a);
            return true;
        });
        runAlgorithm(dfs, diGraph);
        initialized = true;
        arcAdded = false;
        arcRemoved = false;

#ifdef COLLECT_PR_DATA
        parent->prReset();
        parent->prVerticesConsidered(diGraph->getSize());
        parent->prArcsConsidered(diGraph->getNumArcs(true));
#endif
    }

    bool query(const Vertex *t) {
        if (t == source || (initialized && !arcRemoved && dfs.vertexDiscovered(t))) {
            return true;
        } else if (initialized && !arcAdded && !dfs.vertexDiscovered(t)) {
            return false;
        }
        if (!initialized || arcAdded || arcRemoved) {
            run();
        }
        return dfs.vertexDiscovered(t);
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
        if (t == source || (initialized && !arcAdded && !dfs.vertexDiscovered(t))) {
            return;
        } else if (initialized && !arcRemoved && dfs.vertexDiscovered(t)) {
            constructPath(t, path);
            return;
        }
        if (!initialized || arcAdded || arcRemoved) {
            run();
        }

        if (dfs.vertexDiscovered(t)) {
            constructPath(t, path);
        }
    }
};

CachingDFSSSReachAlgorithm::CachingDFSSSReachAlgorithm()
    : DynamicSSReachAlgorithm(), grin(new CheshireCat)
{
    grin->parent = this;
    registerEvents(false, false, true, true);
}

CachingDFSSSReachAlgorithm::~CachingDFSSSReachAlgorithm()
{
    delete grin;
}

void CachingDFSSSReachAlgorithm::run()
{
    grin->run();
}

bool CachingDFSSSReachAlgorithm::query(const Vertex *t)
{
    return grin->query(t);
}

std::vector<Arc *> CachingDFSSSReachAlgorithm::queryPath(const Vertex *t)
{
    std::vector<Arc*> path;
    grin->queryPath(t, path);
    assert (!query(t) || !path.empty() || t == source);
    return path;
}

void CachingDFSSSReachAlgorithm::onDiGraphSet()
{
    DynamicSSReachAlgorithm::onDiGraphSet();
    grin->initialized = false;
    grin->arcAdded = false;
    grin->arcRemoved = false;
    grin->diGraph = diGraph;
}

void CachingDFSSSReachAlgorithm::onDiGraphUnset()
{
    grin->dfs.unsetGraph();
    grin->initialized = false;
    grin->arcAdded = false;
    grin->arcRemoved = false;
    grin->diGraph = nullptr;
    DynamicSSReachAlgorithm::onDiGraphUnset();
}

void CachingDFSSSReachAlgorithm::onArcAdd(Arc *a)
{
    if (!grin->initialized || grin->arcAdded || a->isLoop()) {
        return;
    }

    auto head = a->getHead();

    if (head == source ) {
        return;
    }

    auto tail = a->getTail();

    if (grin->dfs.vertexDiscovered(head) || !grin->dfs.vertexDiscovered(tail)) {
        return;
    }

    grin->arcAdded = true;
}

void CachingDFSSSReachAlgorithm::onArcRemove(Arc *a)
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

void CachingDFSSSReachAlgorithm::onSourceSet()
{
    DynamicSSReachAlgorithm::onSourceSet();
    grin->initialized = false;
    grin->source = source;
}

} // end namespace
