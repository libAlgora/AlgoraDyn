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
    : DynamicSSReachAlgorithm(), initialized(false)
{
    levels.setDefaultValue(-1);
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
}

bool CachingBFSSSReachAlgorithm::query(const Vertex *t)
{
    if (t == source) {
        return true;
    }
    if (!initialized) {
        if (!prepare()) {
            throw DiGraphAlgorithmException(this, "Could not prepare myself.");
        }
        run();
    }
    return levels(t) != -1;
}

void CachingBFSSSReachAlgorithm::onDiGraphSet()
{
    DynamicSSReachAlgorithm::onDiGraphSet();
    bfs.setGraph(diGraph);
    initialized = false;
}

void CachingBFSSSReachAlgorithm::onDiGraphUnset()
{
    bfs.unsetGraph();
    initialized = false;
    DynamicSSReachAlgorithm::onDiGraphUnset();
}

void CachingBFSSSReachAlgorithm::onArcAdd(Arc *)
{
    initialized = false;
}

void CachingBFSSSReachAlgorithm::onArcRemove(Arc *)
{
    initialized = false;
}

void CachingBFSSSReachAlgorithm::onSourceSet()
{
    initialized = false;
}

} // end namespace
