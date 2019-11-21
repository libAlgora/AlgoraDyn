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

#ifndef STATICBFSSSREACHALGORITHM_H
#define STATICBFSSSREACHALGORITHM_H

#include "dynamicsinglesourcereachabilityalgorithm.h"
#include "algorithm.basic/finddipathalgorithm.h"

namespace Algora {

class StaticBFSSSReachAlgorithm : public DynamicSingleSourceReachabilityAlgorithm
{
public:
    explicit StaticBFSSSReachAlgorithm(bool twoWayBFS = false);
    virtual ~StaticBFSSSReachAlgorithm() override = default;

    // DiGraphAlgorithm interface
public:
    virtual void run() override;
    virtual std::string getName() const noexcept override {
        if (twoWayBFS) {
            return "Static BFS Single-Source Reachability Algorithm (forward-backward)";
        }
        return "Static BFS Single-Source Reachability Algorithm (forward-only)";
    }
    virtual std::string getShortName() const noexcept override {
        if (twoWayBFS) {
            return "FB-Static-BFS-SSReach";
        }
        return "Static-BFS-SSReach";
    }
protected:
    virtual void onDiGraphSet() override;

    // DynamicSSReachAlgorithm interface
public:
    virtual bool query(const Vertex *t) override;
    virtual std::vector<Arc*> queryPath(const Vertex *t) override;

private:
    bool twoWayBFS;
    DiGraph::size_type bfsStepSize;
    FindDiPathAlgorithm<FastPropertyMap> fpa;
};

}

#endif // STATICBFSSSREACHALGORITHM_H
