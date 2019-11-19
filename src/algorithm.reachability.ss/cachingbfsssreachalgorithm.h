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

#ifndef CACHINGBFSSSREACHALGORITHM_H
#define CACHINGBFSSSREACHALGORITHM_H

#include "dynamicsinglesourcereachabilityalgorithm.h"

namespace Algora {

class CachingBFSSSReachAlgorithm : public DynamicSingleSourceReachabilityAlgorithm
{
public:
    explicit CachingBFSSSReachAlgorithm();
    virtual ~CachingBFSSSReachAlgorithm();

    // DiGraphAlgorithm interface
public:
    virtual void run() override;
    virtual std::string getName() const noexcept override { return "Caching BFS Single-Source Reachability Algorithm";  }
    virtual std::string getShortName() const noexcept override { return "CachingBFS-SSReach"; }

    // DynamicSSReachAlgorithm interface
    virtual bool query(const Vertex *t) override;
    virtual std::vector<Arc*> queryPath(const Vertex *t) override;

    // DynamicDiGraphAlgorithm interface
    virtual void onArcAdd(Arc *a) override;
    virtual void onArcRemove(Arc *a) override;

protected:
    // DiGraphAlgorithm interface
    virtual void onDiGraphSet() override;
    virtual void onDiGraphUnset() override;

    // DynamicSSReachAlgorithm interface
protected:
    virtual void onSourceSet() override;

private:
    struct CheshireCat;
    CheshireCat *grin;
};

}

#endif // CACHINGBFSSSREACHALGORITHM_H
