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

#ifndef ESTREE_QUEUE_H
#define ESTREE_QUEUE_H

#include "algorithm.reach/dynamicssreachalgorithm.h"
#include "esvertexdata.h"
#include "property/propertymap.h"
#include "property/fastpropertymap.h"
#include <climits>
#include <sstream>
#include <boost/circular_buffer.hpp>

namespace Algora {

class ESTreeQ : public DynamicSSReachAlgorithm
{
public:
    explicit ESTreeQ(unsigned int requeueLimit = UINT_MAX, double maxAffectedRatio = 1.0);
    virtual ~ESTreeQ();
    void setRequeueLimit(unsigned int limit) {
        requeueLimit = limit;
    }
    void setMaxAffectedRatio(double ratio) {
        maxAffectedRatio = ratio;
    }

    // DiGraphAlgorithm interface
public:
    virtual void run() override;
    virtual std::string getName() const noexcept override {
      std::stringstream ss;
            ss << "Queue ES-Tree Single-Source Reachability Algorithm (";
      ss << requeueLimit << "/" << maxAffectedRatio << ")";
      return ss.str();
		}
    virtual std::string getShortName() const noexcept override {
      std::stringstream ss;
            ss << "Q-EST-DSSR(";
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
    virtual void dumpData(std::ostream &os) override;

private:
    FastPropertyMap<ESVertexData*> data;
    FastPropertyMap<unsigned int> inNeighborIndices;
    FastPropertyMap<bool> reachable;
    Vertex *root;
    bool initialized;
    unsigned int requeueLimit;
    double maxAffectedRatio;

    unsigned int movesDown;
    unsigned int movesUp;
    unsigned long long int levelIncrease;
    unsigned long long int levelDecrease;
    unsigned int maxLevelIncrease;
    unsigned int maxLevelDecrease;
    unsigned int decUnreachableHead;
    unsigned int decNonTreeArc;
    unsigned int incUnreachableTail;
    unsigned int incNonTreeArc;
    unsigned int reruns;
    unsigned int maxReQueued;
    unsigned int maxAffected;
    unsigned int totalAffected;

    void restoreTree(ESVertexData *vd);
    void cleanup();
    void dumpTree(std::ostream &os);
    bool checkTree();
    void rerun();
    typedef boost::circular_buffer<ESVertexData*> PriorityQueue;
    unsigned int process(ESVertexData *vd, PriorityQueue &queue,
                     FastPropertyMap<bool> &inQueue,
                     FastPropertyMap<unsigned int> &timesInQueue,
                     bool &limitReached);
};

}

#endif // ESTREE_QUEUE_H
