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

#ifndef SIMPLEESTREE_H
#define SIMPLEESTREE_H

#include "algorithm.reachability.ss/dynamicsinglesourcereachabilityalgorithm.h"
#include "property/propertymap.h"
#include "property/fastpropertymap.h"
#include "sesvertexdata.h"
#include <sstream>
#include <boost/circular_buffer.hpp>

namespace Algora {

class SimpleESTree : public DynamicSingleSourceReachabilityAlgorithm
{
public:
    // requeueLimit, maxAffectedRatio
    typedef std::tuple<unsigned int, double> ParameterSet;

    explicit SimpleESTree(unsigned int requeueLimit = 5, double maxAffectedRatio = .5);
    explicit SimpleESTree(const ParameterSet &params);
    virtual ~SimpleESTree() override;
    void setRequeueLimit(unsigned int limit) {
        requeueLimit = limit;
    }
    void setMaxAffectedRatio(double ratio) {
        maxAffectedRatio = ratio;
    }
		DiGraph::size_type getDepthOfBFSTree() const;
		DiGraph::size_type getNumReachable() const;

    // DiGraphAlgorithm interface
public:
    virtual void run() override;
    virtual std::string getName() const noexcept override {
      std::stringstream ss;
			ss << "Simple ES-Tree Single-Source Reachability Algorithm (";
      ss << requeueLimit << "/" << maxAffectedRatio << ")";
      return ss.str();
		}
    virtual std::string getShortName() const noexcept override {
      std::stringstream ss;
            ss << "Simple-EST-DSSR(";
      ss << requeueLimit << "/" << maxAffectedRatio << ")";
      return ss.str();
		}
    virtual std::string getProfilingInfo() const override;
    virtual Profile getProfile() const override;

    // DynamicDiGraphAlgorithm interface
public:
    virtual void onVertexAdd(Vertex *v) override;
    virtual void onArcAdd(Arc *a) override;
    virtual void onVertexRemove(Vertex *v) override;
    virtual void onArcRemove(Arc *a) override;

protected:
    virtual void onDiGraphSet() override;
    virtual void onDiGraphUnset() override;

    // DynamicSSReachAlgorithm interface
protected:
    virtual void onSourceSet() override;

    // DynamicSSReachAlgorithm interface
public:
    virtual bool query(const Vertex *t) override;
    virtual std::vector<Arc*> queryPath(const Vertex *t) override;
    virtual void dumpData(std::ostream &os) const override;

private:
    typedef boost::circular_buffer<SESVertexData*> PriorityQueue;

    FastPropertyMap<SESVertexData*> data;
    FastPropertyMap<bool> reachable;
    FastPropertyMap<unsigned int> timesInQueue;
    PriorityQueue queue;

    Vertex *root;
    bool initialized;
    unsigned int requeueLimit;
    double maxAffectedRatio;

    profiling_counter movesDown;
    profiling_counter movesUp;
    profiling_counter levelIncrease;
    profiling_counter levelDecrease;
    DiGraph::size_type maxLevelIncrease;
    DiGraph::size_type maxLevelDecrease;
    profiling_counter decUnreachableHead;
    profiling_counter decNonTreeArc;
    profiling_counter incUnreachableTail;
    profiling_counter incNonTreeArc;
    profiling_counter reruns;
    unsigned int maxReQueued;
    DiGraph::size_type maxAffected;
    profiling_counter totalAffected;
    profiling_counter rerunRequeued;
    profiling_counter rerunNumAffected;

    void restoreTree(SESVertexData *rd);
    void cleanup(bool freeSpace);
    void dumpTree(std::ostream &os);
    bool checkTree();
    void rerun();
    DiGraph::size_type process(SESVertexData *vd, bool &limitReached);
};

}

#endif // SIMPLEESTREE_H
