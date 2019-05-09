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

#ifndef DYNAMICSSREACHALGORITHM_H
#define DYNAMICSSREACHALGORITHM_H

#include "algorithm/dynamicdigraphalgorithm.h"
#include "graph/digraph.h"
#include <ostream>
#include <sstream>

namespace Algora {

class DynamicSSReachAlgorithm : public DynamicDiGraphAlgorithm
{
public:
    typedef unsigned long long pr_val;
    typedef std::vector<std::pair<std::string, pr_val>> Profile;

    explicit DynamicSSReachAlgorithm() : DynamicDiGraphAlgorithm(), source(nullptr) { }
    virtual ~DynamicSSReachAlgorithm() override { }

    void setSource(Vertex *s) { source = s; onSourceSet(); }
    virtual bool query(const Vertex *t) = 0;
    virtual std::vector<Arc*> queryPath(const Vertex *) { return std::vector<Arc*>(); }

    virtual void dumpData(std::ostream&) { }
    virtual Profile getProfile() const {
        return Profile {
            std::pair(std::string("vertices_considered"), pr_consideredVertices),
            std::pair(std::string("arcs_considered"), pr_consideredArcs),
            std::pair(std::string("num_resets"), pr_numResets)
                    };
    }

    virtual void ping() { }

    // DiGraphAlgorithm interface
public:
    virtual bool prepare() override { return source != nullptr && DynamicDiGraphAlgorithm::prepare() && diGraph->containsVertex(source); }
    virtual std::string getProfilingInfo() const override {
        std::stringstream ss;
        for (const auto &[key, value] : getProfile()) {
            ss << key << ": " << value << '\n';
        }
        return ss.str();
    }

protected:
    virtual void onSourceSet() { }
    virtual void onDiGraphSet() override { DynamicDiGraphAlgorithm::onDiGraphSet(); resetProfileData(); }

    void prVertexConsidered() { pr_consideredVertices++; }
    void prVerticesConsidered(const pr_val &n) { pr_consideredVertices += n; }
    void prArcConsidered() { pr_consideredArcs++; }
    void prArcsConsidered(const pr_val &m) { pr_consideredArcs += m; }
    void prReset() { pr_numResets++; }
    void resetProfileData() { pr_consideredVertices = 0UL; pr_consideredArcs = 0UL; pr_numResets = 0UL; }

    Vertex *source;

    pr_val pr_consideredVertices;
    pr_val pr_consideredArcs;
    pr_val pr_numResets;

};

}

#endif // DYNAMICSSREACHALGORITHM_H
