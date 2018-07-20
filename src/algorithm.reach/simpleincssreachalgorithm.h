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

#ifndef SIMPLEINCSSREACHALGORITHM_H
#define SIMPLEINCSSREACHALGORITHM_H

#include "dynamicssreachalgorithm.h"

#include "property/propertymap.h"

namespace Algora {

class SimpleIncSSReachAlgorithm : public DynamicSSReachAlgorithm
{
public:
    explicit SimpleIncSSReachAlgorithm(bool reverse = false, bool searchForward = false);
    virtual ~SimpleIncSSReachAlgorithm();

    // DiGraphAlgorithm interface
public:
    virtual void run() override;
    virtual std::string getName() const noexcept override {
        return reverse ? ( searchForward ? "Simple Incremental Single-Source Reachability Algorithm (reverse, search forward)"
                                            : "Simple Incremental Single-Source Reachability Algorithm (reverse)")
                        :  ( searchForward ? "Simple Incremental Single-Source Reachability Algorithm (search forward)"
                                            : "Simple Incremental Single-Source Reachability Algorithm");
    }
    virtual std::string getShortName() const noexcept override {
       return  reverse ? ( searchForward ? "Simple-ISSReach-R-SF"
                                            : "Simple-ISSReach-R")
                        :  ( searchForward ? "Simple-ISSReach-SF"
                                            : "Simple-ISSReach");
    }
    virtual std::string getProfilingInfo() const override;
    virtual Profile getProfile() const override;

protected:
    virtual void onDiGraphUnset() override;

    // DynamicDiGraphAlgorithm interface
protected:
    virtual void onVertexAdd(Vertex *) override;
    virtual void onVertexRemove(Vertex *v) override;
    virtual void onArcAdd(Arc *a) override;
    virtual void onArcRemove(Arc *a) override;

    // DynamicSSReachAlgorithm interface
public:
    virtual bool query(const Vertex *t) override;
    virtual void dumpData(std::ostream &os) override;

    // DynamicSSReachAlgorithm interface
protected:
    virtual void onSourceSet() override;

private:
    struct Reachability;
    Reachability *data;
    bool initialized;

    bool reverse;
    bool searchForward;
};

}

#endif // SIMPLEINCSSREACHALGORITHM_H
