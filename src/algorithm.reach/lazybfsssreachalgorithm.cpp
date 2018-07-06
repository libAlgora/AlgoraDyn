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

#include "lazybfsssreachalgorithm.h"

#include "property/fastpropertymap.h"

#include <boost/circular_buffer.hpp>

namespace Algora {

struct LazyBFSSSReachAlgorithm::CheshireCat {
    bool inititialized;
    DiGraph *graph;
    Vertex *source;
    boost::circular_buffer<Vertex*> queue;
    FastPropertyMap<bool> discovered;

    CheshireCat()
        : inititialized(false) { discovered.setDefaultValue(false); }

    void searchOn(const Vertex *t) {
        if (!inititialized) {
            queue.clear();
            queue.set_capacity(graph->getSize());
            queue.push_back(source);
            discovered.resetAll();
            discovered[source] = true;
        }
        bool stop = false;
        while (!stop && !queue.empty()) {
            const Vertex *v = queue.front();
            queue.pop_front();
            graph->mapOutgoingArcs(v, [&](Arc *a) {
                Vertex *head = a->getHead();
                if (!discovered(head)) {
                    queue.push_back(head);
                    discovered[head] = true;
                    if (head == t) {
                        stop = true;
                    }
                }
            });
        }
        inititialized = true;
    }
};

LazyBFSSSReachAlgorithm::LazyBFSSSReachAlgorithm()
    : grin(new CheshireCat)
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
    grin->inititialized = false;
    grin->graph = diGraph;
}

void LazyBFSSSReachAlgorithm::onArcAdd(Arc *)
{
    grin->inititialized = false;
}

void LazyBFSSSReachAlgorithm::onArcRemove(Arc *)
{
    grin->inititialized = false;
}

bool LazyBFSSSReachAlgorithm::query(const Vertex *t)
{
    if (t == source || (grin->inititialized && grin->discovered(t))) {
        return true;
    }
    grin->searchOn(t);
    return grin->discovered(t);
}

void LazyBFSSSReachAlgorithm::onSourceSet()
{
    DynamicSSReachAlgorithm::onSourceSet();
    grin->inititialized = false;
    grin->source = source;
}

}
