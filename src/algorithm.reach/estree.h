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

#ifndef ESTREE_H
#define ESTREE_H

#include "dynamicssreachalgorithm.h"
#include "property/propertymap.h"

namespace Algora {

class ESTree : public DynamicSSReachAlgorithm
{
public:
    struct VertexData;
    explicit ESTree();
    virtual ~ESTree();

    // DiGraphAlgorithm interface
public:
    virtual void run() override;
    virtual std::string getName() const noexcept override { return "ES-Tree Single-Source Reachability Algorithm"; }
    virtual std::string getShortName() const noexcept override { return "EST-DSSReach"; }
    virtual std::string getProfilingInfo() const override;

protected:
    virtual void onDiGraphUnset() override;

    // DynamicDiGraphAlgorithm interface
protected:
    virtual void onVertexAdd(Vertex *v) override;
    virtual void onArcAdd(Arc *a) override;
    virtual void onVertexRemove(Vertex *v) override;
    virtual void onArcRemove(Arc *a) override;

    // DynamicSSReachAlgorithm interface
protected:
    virtual void onSourceSet() override;

    // DynamicSSReachAlgorithm interface
public:
    virtual bool query(const Vertex *t) override;
    virtual void dumpData(std::ostream &os) override;

private:
    PropertyMap<VertexData*> data;
    PropertyMap<bool> reachable;
    Vertex *root;
    bool initialized;

    unsigned int movesDown;
    unsigned int movesUp;
    unsigned int levelIncrease;
    unsigned int decUnreachableHead;
    unsigned int decNonTreeArc;
    unsigned int incUnreachableTail;
    unsigned int incNonTreeArc;

    void restoreTree(VertexData *rd);
    void cleanup();

};

}

#endif // ESTREE_H
