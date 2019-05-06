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

#include "dynamicssreachalgorithm.h"
#include <sstream>

#include "property/propertymap.h"

namespace Algora {

class SimpleIncSSReachAlgorithm : public DynamicSSReachAlgorithm
{
public:
    explicit SimpleIncSSReachAlgorithm(bool reverse = false, bool searchForward = false, double maxUS = 1.0, bool radicalReset = false);
    virtual ~SimpleIncSSReachAlgorithm() override;
    void setMaxUnknownStateSqrt();
    void setMaxUnknownStateLog();
    /** relate unknown state ratio to #reachable vertices or #all vertices **/
    void relateToReachableVertices(bool relReachable);

    // DiGraphAlgorithm interface
public:
    virtual void run() override;
    virtual std::string getName() const noexcept override {
        std::stringstream ss;
        ss << "Simple Incremental Single-Source Reachability Algorithm ("
            << (reverse ? "reverse" : "non-reverse") << "/"
            << (searchForward ? "forward search" : "no forward search") << "/";
        if (maxUSSqrt) {
            ss << "SQRT";
        } else if (maxUSLog) {
            ss << "LOG";
        } else {
            ss << maxUnknownStateRatio;
        }
        ss << "*" << (relateToReachable ? "#R" : "#V") << "/";
        ss  << (radicalReset? "radical reset" : "soft reset") << ")";
        return ss.str();
    }
    virtual std::string getShortName() const noexcept override {
        std::stringstream ss;
        ss << "Simple-ISSR("
            << (reverse ? "R" : "NR") << "/"
            << (searchForward ? "SF" : "NSF") << "/";
        if (maxUSSqrt) {
            ss << "SQRT";
        } else if (maxUSLog) {
            ss << "LOG";
        } else {
            ss << maxUnknownStateRatio;
        }
        ss << "~" << (relateToReachable ? "R" : "G") << "/";
        ss << (radicalReset ? "C" : "NC") << ")";
        return ss.str();
    }
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
    virtual void dumpData(std::ostream &os) override;

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
