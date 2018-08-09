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

#ifndef CACHINGDFSSSREACHALGORITHM_H
#define CACHINGDFSSSREACHALGORITHM_H

#include "algorithm.reach/dynamicssreachalgorithm.h"
#include "algorithm.basic.traversal/depthfirstsearch.h"
#include "property/fastpropertymap.h"

namespace Algora {

class CachingDFSSSReachAlgorithm : public DynamicSSReachAlgorithm
{
public:
    explicit CachingDFSSSReachAlgorithm();
    virtual ~CachingDFSSSReachAlgorithm();

    // DiGraphAlgorithm interface
public:
    virtual void run() override;
    virtual std::string getName() const noexcept override { return "Caching DFS Single-Source Reachability Algorithm";  }
    virtual std::string getShortName() const noexcept override { return "CachingDFS-SSReach"; }

    // DynamicSSReachAlgorithm interface
    virtual bool query(const Vertex *t) override;

protected:
    // DiGraphAlgorithm interface
    virtual void onDiGraphSet() override;
    virtual void onDiGraphUnset() override;

    // DynamicDiGraphAlgorithm interface
    virtual void onArcAdd(Arc *a) override;
    virtual void onArcRemove(Arc *a) override;

    // DynamicSSReachAlgorithm interface
protected:
    virtual void onSourceSet() override;

private:
    DepthFirstSearch<FastPropertyMap> dfs;
    FastPropertyMap<DFSResult> dfsResults;
    bool initialized;
    bool arcAdded;
    bool arcRemoved;
};

}

#endif // CACHINGDFSSSREACHALGORITHM_H
