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

#include "lazydfsssreachalgorithm.h"

#include "property/fastpropertymap.h"

#include <boost/circular_buffer.hpp>

namespace Algora {

struct LazyDFSSSReachAlgorithm::CheshireCat {
    bool initialized;
    bool arcAdded;
    bool arcRemoved;
    bool exhausted;
    DiGraph *graph;
    Vertex *source;
    std::vector<Arc*> stack;
    FastPropertyMap<bool> discovered;

    CheshireCat()
        : initialized(false), arcAdded(false), arcRemoved(false), exhausted(false) { discovered.setDefaultValue(false); }

    bool searchOn(const Vertex *t) {
        if (!initialized) {
            stack.clear();
            graph->mapOutgoingArcs(source, [&](Arc *a) {
                stack.push_back(a);
            });
            discovered.resetAll();
            discovered[source] = true;
        }
        bool stop = false;
        while (!stop && !stack.empty()) {
            const Arc *a = stack.back();
            stack.pop_back();
            Vertex *v = a->getHead();
            if (discovered(v)) {
                continue;
            }
            discovered[v] = true;
            if (v == t) {
                stop = true;
            }

            graph->mapOutgoingArcs(v, [&](Arc *a) {
                Vertex *head = a->getHead();
                if (!discovered(head)) {
                    stack.push_back(a);
                    if (head == t) {
                      stop = true;
                    }
                }
            });
        }
        initialized = true;
        arcAdded = false;
        arcRemoved = false;
        exhausted = stack.empty();
        return stop;
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
        return searchOn(t) || discovered(t);
    }
};

LazyDFSSSReachAlgorithm::LazyDFSSSReachAlgorithm()
    : grin(new CheshireCat)
{

}

LazyDFSSSReachAlgorithm::~LazyDFSSSReachAlgorithm()
{
    delete grin;
}

void LazyDFSSSReachAlgorithm::run()
{

}

void LazyDFSSSReachAlgorithm::onDiGraphSet()
{
    DynamicSSReachAlgorithm::onDiGraphSet();
    grin->initialized = false;
    grin->arcAdded = false;
    grin->arcRemoved = false;
    grin->graph = diGraph;
}

void LazyDFSSSReachAlgorithm::onVertexAdd(Vertex *v)
{
    grin->discovered[v] = false;
    grin->exhausted = false;
}

void LazyDFSSSReachAlgorithm::onVertexRemove(Vertex *v)
{
    grin->discovered.resetToDefault(v);
}

void LazyDFSSSReachAlgorithm::onArcAdd(Arc *a)
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

void LazyDFSSSReachAlgorithm::onArcRemove(Arc *a)
{
    if (a->isLoop()) {
        return;
    }

    auto head = a->getHead();

    if (head == source || grin->arcRemoved || !grin->initialized) {
        return;
    }

    if (grin->exhausted && !grin->discovered(head)) {
        return;
    }

    grin->arcRemoved = true;
}

bool LazyDFSSSReachAlgorithm::query(const Vertex *t)
{
    return grin->query(t);
}

void LazyDFSSSReachAlgorithm::onSourceSet()
{
    DynamicSSReachAlgorithm::onSourceSet();
    grin->initialized = false;
    grin->arcAdded = false;
    grin->arcRemoved = false;
    grin->source = source;
}

}
