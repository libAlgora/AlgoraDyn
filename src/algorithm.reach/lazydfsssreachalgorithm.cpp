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
    bool inititialized;
    DiGraph *graph;
    Vertex *source;
    std::vector<Arc*> stack;
    FastPropertyMap<bool> discovered;

    CheshireCat()
        : inititialized(false) { discovered.setDefaultValue(false); }

    bool searchOn(const Vertex *t) {
        if (!inititialized) {
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
        inititialized = true;
        return stop;
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
    grin->inititialized = false;
    grin->graph = diGraph;
}

void LazyDFSSSReachAlgorithm::onArcAdd(Arc *)
{
    grin->inititialized = false;
}

void LazyDFSSSReachAlgorithm::onArcRemove(Arc *)
{
    grin->inititialized = false;
}

bool LazyDFSSSReachAlgorithm::query(const Vertex *t)
{
    if (t == source || (grin->inititialized && grin->discovered(t))) {
        return true;
    }
    return grin->searchOn(t) || grin->discovered(t);
}

void LazyDFSSSReachAlgorithm::onSourceSet()
{
    DynamicSSReachAlgorithm::onSourceSet();
    grin->inititialized = false;
    grin->source = source;
}

}
