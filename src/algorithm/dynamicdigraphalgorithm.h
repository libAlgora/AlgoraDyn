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

#ifndef DYNAMICDIGRAPHALGORITHM_H
#define DYNAMICDIGRAPHALGORITHM_H

#include "algorithm/digraphalgorithm.h"

#include <vector>

namespace Algora {

class Vertex;
class Arc;

class DynamicDiGraphAlgorithm
        : public DiGraphAlgorithm
{
public:
    typedef unsigned long long profiling_counter;
    typedef std::vector<std::pair<std::string, profiling_counter>> Profile;

    explicit DynamicDiGraphAlgorithm();
    virtual ~DynamicDiGraphAlgorithm() override;

    virtual void setAutoUpdate(bool au) {
        if (!au && registered) {
            deregisterAsObserver();
        } else if (au && !registered) {
            registerAsObserver();
        }
        this->autoUpdate = au;
    }

    bool doesAutoUpdate() const {
        return this->autoUpdate;
    }

    virtual void onVertexAdd(Vertex *) { }
    virtual void onVertexRemove(Vertex *) { }
    virtual void onArcAdd(Arc *) { }
    virtual void onArcRemove(Arc *) { }

    virtual void dumpData(std::ostream&) const { }
    virtual Profile getProfile() const;

    virtual void ping() { }

    // DiGraphAlgorithm interface
public:
    virtual std::string getProfilingInfo() const override;

protected:
    profiling_counter pr_consideredVertices;
    profiling_counter pr_consideredArcs;
    profiling_counter pr_numResets;

    virtual void onDiGraphSet() override;
    virtual void onDiGraphUnset() override;

    void registerEvents(bool vertexAdd, bool vertexRemove, bool arcAdd, bool arcRemove) {
        registerOnVertexAdd = vertexAdd;
        registerOnVertexRemove = vertexRemove;
        registerOnArcAdd = arcAdd;
        registerOnArcRemove = arcRemove;
    }

    //inline
    void prVertexConsidered() { pr_consideredVertices++; }
    void prVerticesConsidered(const profiling_counter &n) { pr_consideredVertices += n; }
    void prArcConsidered() { pr_consideredArcs++; }
    void prArcsConsidered(const profiling_counter &m) { pr_consideredArcs += m; }
    void prReset() { pr_numResets++; }
    void resetProfileData() {
        pr_consideredVertices = 0UL; pr_consideredArcs = 0UL; pr_numResets = 0UL;
    }


private:
    bool autoUpdate;
    bool registered;

    bool registerOnVertexAdd;
    bool registerOnVertexRemove;
    bool registerOnArcAdd;
    bool registerOnArcRemove;

    void registerAsObserver();
    void deregisterAsObserver();
};

}

#endif // DYNAMICDIGRAPHALGORITHM_H
