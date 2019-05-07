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

#include "dynamicssreachalgorithm.h"

namespace Algora {

class StaticBFSSSReachAlgorithm : public DynamicSSReachAlgorithm
{
public:
    explicit StaticBFSSSReachAlgorithm();
    virtual ~StaticBFSSSReachAlgorithm();

    // DiGraphAlgorithm interface
public:
    virtual void run() override;
    virtual std::string getName() const noexcept override { return "Static BFS Single-Source Reachability Algorithm"; }
    virtual std::string getShortName() const noexcept override { return "Static-BFS-SSReach"; }

    // DynamicSSReachAlgorithm interface
public:
    virtual bool query(const Vertex *t) override;
    virtual std::vector<Arc*> queryPath(const Vertex *t) override;
};

}

#endif // STATICBFSSSREACHALGORITHM_H
