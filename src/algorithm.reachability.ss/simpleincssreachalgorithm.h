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

#ifndef SIMPLEINCSSREACHALGORITHM_H
#define SIMPLEINCSSREACHALGORITHM_H

#include "dynamicsinglesourcereachabilityalgorithm.h"
#include <sstream>

#include "property/propertymap.h"

namespace Algora {

class SimpleIncSSReachAlgorithm : public DynamicSingleSourceReachabilityAlgorithm
{
public:
    // reverse, searchForward, maxUnknownRatio, radicalReset, maxUnknownSqrt, maxUnknownLog,
    // releateToReachable
    typedef std::tuple<bool, bool, double, bool, bool, bool, bool> ParameterSet;

    explicit SimpleIncSSReachAlgorithm(bool reverse = false,
                                       bool searchForward = true,
                                       double maxUS = 0.25,
                                       bool radicalReset = false);
    explicit SimpleIncSSReachAlgorithm(const ParameterSet &params);
    virtual ~SimpleIncSSReachAlgorithm() override;
    void setMaxUnknownStateSqrt();
    void setMaxUnknownStateLog();
    /** relate unknown state ratio to #reachable vertices or #all vertices **/
    void relateToReachableVertices(bool relReachable);

    // DiGraphAlgorithm interface
public:
    virtual void run() override;
    virtual std::string getName() const noexcept override;
    virtual std::string getShortName() const noexcept override;
    virtual std::string getProfilingInfo() const override;
    virtual Profile getProfile() const override;

    // DynamicDiGraphAlgorithm interface
public:
    virtual void onVertexAdd(Vertex *) override;
    virtual void onVertexRemove(Vertex *v) override;
    virtual void onArcAdd(Arc *a) override;
    virtual void onArcRemove(Arc *a) override;

protected:
    virtual void onDiGraphSet() override;
    virtual void onDiGraphUnset() override;

    // DynamicSSReachAlgorithm interface
public:
    virtual bool query(const Vertex *t) override;
    virtual std::vector<Arc*> queryPath(const Vertex *t) override;
    virtual void dumpData(std::ostream &os) const override;

    // DynamicSSReachAlgorithm interface
protected:
    virtual void onSourceSet() override;

private:
    bool initialized;

    bool reverse;
    bool searchForward;
    double maxUnknownStateRatio;
    bool maxUSSqrt;
    bool maxUSLog;
    bool relateToReachable;
    bool radicalReset;

    struct Reachability;
    Reachability *data;
};

}

#endif // SIMPLEINCSSREACHALGORITHM_H
