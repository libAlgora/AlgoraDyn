/**
 * Copyright (C) 2013 - 2020 : Kathrin Hanauer
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

#ifndef STATICDFSAPREACHALGORITHM_H
#define STATICDFSAPREACHALGORITHM_H

#include "dynamicallpairsreachabilityalgorithm.h"

namespace Algora {

class StaticDFSAPReachabilityAlgorithm : public DynamicAllPairsReachabilityAlgorithm
{
public:
    explicit StaticDFSAPReachabilityAlgorithm();
    virtual ~StaticDFSAPReachabilityAlgorithm() override;

    // DiGraphAlgorithm interface
public:
    virtual void run() override;
    virtual std::string getName() const noexcept override { return "Static DFS All-Pairs Reachability Algorithm"; }
    virtual std::string getShortName() const noexcept override { return "Static-DFS-APReach"; }

    // DynamicAPReachabilityAlgorithm interface
public:
    virtual bool query(Vertex *s, Vertex *t) override;
    virtual std::vector<Arc*> queryPath(Vertex *s, Vertex *t) override;
};

}

#endif // STATICDFSSSREACHALGORITHM_H
