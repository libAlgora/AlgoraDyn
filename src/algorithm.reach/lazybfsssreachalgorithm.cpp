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

#include "lazybfsssreachalgorithm.h"

#include "property/fastpropertymap.h"

#include <boost/circular_buffer.hpp>

namespace Algora {

struct LazyBFSSSReachAlgorithm::CheshireCat {
    LazyBFSSSReachAlgorithm *parent;
    bool initialized;
    bool arcAdded;
    bool arcRemoved;
    bool exhausted;
    DiGraph *graph;
    Vertex *source;
    boost::circular_buffer<Vertex*> queue;
    FastPropertyMap<bool> discovered;
    FastPropertyMap<Arc*> treeArc;

    CheshireCat(LazyBFSSSReachAlgorithm *p)
        : parent(p), initialized(false), arcAdded(false), arcRemoved(false), exhausted(false) {
        discovered.setDefaultValue(false);
        treeArc.setDefaultValue(nullptr);
    }

    void searchOn(const Vertex *t) {
        if (!initialized) {
            queue.clear();
            queue.set_capacity(graph->getSize());
            queue.push_back(source);
            discovered.resetAll();
            treeArc.resetAll();
            discovered[source] = true;
#ifdef COLLECT_PR_DATA
            parent->prReset();
#endif
        }
        bool stop = false;
        while (!stop && !queue.empty()) {
            const Vertex *v = queue.front();
#ifdef COLLECT_PR_DATA
                parent->prVertexConsidered();
#endif
            queue.pop_front();
            graph->mapOutgoingArcs(v, [&](Arc *a) {
#ifdef COLLECT_PR_DATA
                parent->prArcConsidered();
#endif
                Vertex *head = a->getHead();
                if (!discovered(head)) {
                    queue.push_back(head);
                    discovered[head] = true;
                    treeArc[head] = a;
                    if (head == t) {
                        stop = true;
                    }
                }
            });
        }
        initialized = true;
        arcAdded = false;
        arcRemoved = false;
        exhausted = queue.empty();
    }

    bool query(const Vertex *t) {
        if (t == source || (initialized && !arcRemoved && discovered(t))) {
            return true;
        } else if (exhausted && !arcAdded && !discovered(t)) {
            return false;
        }
        if (arcAdded || arcRemoved) {
            initialized = false;
        }
        searchOn(t);
        return discovered(t);
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
        if (t == source || (exhausted && !arcAdded && !discovered(t))) {
            return;
        } else if (initialized && !arcRemoved && discovered(t)) {
            constructPath(t, path);
            return;
        }
        if (arcAdded || arcRemoved) {
            initialized = false;
        }

        searchOn(t);
        if (discovered(t)) {
            constructPath(t, path);
        }
    }
};

LazyBFSSSReachAlgorithm::LazyBFSSSReachAlgorithm()
    : grin(new CheshireCat(this))
{

}

LazyBFSSSReachAlgorithm::~LazyBFSSSReachAlgorithm()
{
    delete grin;
}

void LazyBFSSSReachAlgorithm::run()
{

}

void LazyBFSSSReachAlgorithm::onDiGraphSet()
{
    DynamicSSReachAlgorithm::onDiGraphSet();
    grin->initialized = false;
    grin->arcAdded = false;
    grin->arcRemoved = false;
    grin->graph = diGraph;
}

void LazyBFSSSReachAlgorithm::onVertexAdd(Vertex *v)
{
    grin->discovered[v] = false;
    grin->exhausted = false;
}

void LazyBFSSSReachAlgorithm::onVertexRemove(Vertex *v)
{
    grin->discovered.resetToDefault(v);
}

void LazyBFSSSReachAlgorithm::onArcAdd(Arc *a)
{
    if (a->isLoop()) {
        return;
    }

    auto head = a->getHead();

    if (head == source || grin->arcAdded || !grin->initialized) {
        return;
    }

    auto tail = a->getTail();

    if (grin->discovered(head) || (grin->exhausted && !grin->discovered(tail))) {
        return;
    }

    grin->arcAdded = true;
}

void LazyBFSSSReachAlgorithm::onArcRemove(Arc *a)
{
    if (a->isLoop()) {
        return;
    }

    auto head = a->getHead();

    if (head == source || grin->arcRemoved || !grin->initialized) {
        return;
    }

    if ((grin->exhausted || grin->discovered(head)) && a != grin->treeArc(head)) {
        return;
    }

    grin->arcRemoved = true;
}

bool LazyBFSSSReachAlgorithm::query(const Vertex *t)
{
    return grin->query(t);
}

std::vector<Arc *> LazyBFSSSReachAlgorithm::queryPath(const Vertex *t)
{
    std::vector<Arc*> path;
    grin->queryPath(t, path);
    assert (!query(t) || !path.empty() || t == source);
    return path;
}

void LazyBFSSSReachAlgorithm::onSourceSet()
{
    DynamicSSReachAlgorithm::onSourceSet();
    grin->initialized = false;
    grin->arcAdded = false;
    grin->arcRemoved = false;
    grin->source = source;
}

}
