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

#ifndef ESTREE_BQUEUE_H
#define ESTREE_BQUEUE_H

#include "algorithm.reach/dynamicssreachalgorithm.h"
#include "esvertexdata.h"
#include "datastructure/bucketqueue.h"
#include "property/propertymap.h"
#include "property/fastpropertymap.h"
#include <climits>
#include <sstream>

namespace Algora {

class OldESTree : public DynamicSSReachAlgorithm
{
public:
    explicit OldESTree(unsigned long long requeueLimit = UINT_MAX, double maxAffectedRatio = 1.0);
    virtual ~OldESTree();
    void setRequeueLimit(unsigned long long limit) {
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
        ss << "BucketQueue ES-Tree Single-Source Reachability Algorithm (";
        ss << requeueLimit << "/" << maxAffectedRatio << ")";
        return ss.str();
    }
    virtual std::string getShortName() const noexcept override {
        std::stringstream ss;
        ss << "BQ-EST-DSSR(";
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
    FastPropertyMap<unsigned long long> inNeighborIndices;
    FastPropertyMap<bool> reachable;
    Vertex *root;
    bool initialized;
    unsigned long long requeueLimit;
    double maxAffectedRatio;

    unsigned long long  movesDown;
    unsigned long long  movesUp;
    unsigned long long levelIncrease;
    unsigned long long levelDecrease;
    unsigned long long maxLevelIncrease;
    unsigned long long maxLevelDecrease;
    unsigned long long decUnreachableHead;
    unsigned long long decNonTreeArc;
    unsigned long long incUnreachableTail;
    unsigned long long incNonTreeArc;
    unsigned long long reruns;
    unsigned long long maxReQueued;
    unsigned long long maxAffected;
    unsigned long long totalAffected;

    void restoreTree(ESVertexData *vd);
    void cleanup();
    void dumpTree(std::ostream &os);
    bool checkTree();
    void rerun();
    typedef BucketQueue<ESVertexData*, ES_Priority> PriorityQueue;
    unsigned long long process(ESVertexData *vd, PriorityQueue &queue,
                     FastPropertyMap<bool> &inQueue,
                     FastPropertyMap<unsigned long long> &timesInQueue,
                     bool &limitReached);
};

}

#endif // ESTREE_BQUEUE_H