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

#ifndef DYNAMICSSREACHALGORITHM_H
#define DYNAMICSSREACHALGORITHM_H

#include "algorithm/dynamicdigraphalgorithm.h"
#include "graph/digraph.h"
#include <ostream>

namespace Algora {

class DynamicSSReachAlgorithm : public DynamicDiGraphAlgorithm
{
public:
    typedef std::vector<std::pair<std::string, unsigned int>> Profile;

    explicit DynamicSSReachAlgorithm() : DynamicDiGraphAlgorithm(), source(nullptr) { }
    virtual ~DynamicSSReachAlgorithm() { }

    void setSource(Vertex *s) { source = s; onSourceSet(); }
    virtual bool query(const Vertex *t) = 0;

    virtual void dumpData(std::ostream&) { }
    virtual Profile getProfile() const { return Profile(); }

    // DiGraphAlgorithm interface
public:
    virtual bool prepare() override { return source != nullptr && DynamicDiGraphAlgorithm::prepare() && diGraph->containsVertex(source); }

protected:
    virtual void onSourceSet() { }

    Vertex *source;
};

}

#endif // DYNAMICSSREACHALGORITHM_H
