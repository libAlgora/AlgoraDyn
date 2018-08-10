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

#include "cachingdfsssreachalgorithm.h"
#include "algorithm/digraphalgorithmexception.h"

namespace Algora {

CachingDFSSSReachAlgorithm::CachingDFSSSReachAlgorithm()
    : DynamicSSReachAlgorithm(), initialized(false),
      arcAdded(false), arcRemoved(false)
{
    dfsResults.setDefaultValue(DFSResult());
    dfs.useModifiableProperty(&dfsResults);
}

CachingDFSSSReachAlgorithm::~CachingDFSSSReachAlgorithm()
{

}

void CachingDFSSSReachAlgorithm::run()
{
    dfsResults.resetAll(diGraph->getSize());
    dfs.setStartVertex(source);
    if (!dfs.prepare()) {
       throw DiGraphAlgorithmException(this, "Could not prepare DFS algorithm.");
    }
    dfs.run();
    dfs.deliver();
    initialized = true;
    arcAdded = false;
    arcRemoved = false;
}

bool CachingDFSSSReachAlgorithm::query(const Vertex *t)
{
    if (t == source || (initialized && !arcRemoved && dfsResults[t].dfsNumber >= 0)) {
        return true;
    } else if (initialized && !arcAdded && dfsResults[t].dfsNumber < 0) {
        return false;
    }
    if (!initialized || arcAdded || arcRemoved) {
        if (!prepare()) {
            throw DiGraphAlgorithmException(this, "Could not prepare myself.");
        }
        run();
    }
    return dfsResults[t].dfsNumber >= 0;
}

void CachingDFSSSReachAlgorithm::onDiGraphSet()
{
    DynamicSSReachAlgorithm::onDiGraphSet();
    dfs.setGraph(diGraph);
    dfsResults.resetAll(diGraph->getSize());
    initialized = false;
    arcAdded = false;
    arcRemoved = false;
}

void CachingDFSSSReachAlgorithm::onDiGraphUnset()
{
    dfs.unsetGraph();
    initialized = false;
    arcAdded = false;
    arcRemoved = false;
    DynamicSSReachAlgorithm::onDiGraphUnset();
}

void CachingDFSSSReachAlgorithm::onArcAdd(Arc *a)
{
    if (!initialized || arcAdded || a->isLoop()) {
        return;
    }

    auto head = a->getHead();

    if (head == source ) {
        return;
    }

    auto tail = a->getTail();

    if (dfsResults[head].dfsNumber >= 0 || dfsResults[tail].dfsNumber < 0) {
        return;
    }

    arcAdded = true;
}

void CachingDFSSSReachAlgorithm::onArcRemove(Arc *a)
{
    if (!initialized || arcRemoved || a->isLoop()) {
        return;
    }

    auto head = a->getHead();

    if (head == source) {
        return;
    }

    if (dfsResults[head].dfsNumber < 0) {
        return;
    }

    arcRemoved = true;
}

void CachingDFSSSReachAlgorithm::onSourceSet()
{
    DynamicSSReachAlgorithm::onSourceSet();
    initialized = false;
}

} // end namespace
