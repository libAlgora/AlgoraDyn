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

#ifndef ESVERTEXDATA_H
#define ESVERTEXDATA_H

#include <vector>
#include <climits>
#include <iostream>
#include <property/fastpropertymap.h>

namespace Algora {

class DynamicDiGraphAlgorithm;
class Vertex;
class Arc;

class ESVertexData
{
    friend std::ostream& operator<<(std::ostream &os, const ESVertexData *vd);

public:
    static constexpr unsigned long long UNREACHABLE = ULLONG_MAX;

    ESVertexData(FastPropertyMap<unsigned long long> *inNeighborIndices,
            Vertex *v, ESVertexData *p = nullptr, const Arc *treeArc = nullptr, unsigned long long l = UNREACHABLE);

    void reset(ESVertexData *p = nullptr, const Arc *treeArc = nullptr, unsigned long long l = UNREACHABLE);

    void setUnreachable();
    bool isReachable() const;
    unsigned long long getLevel() const {
        return isReachable() ? level : UNREACHABLE;
    }
    Vertex *getVertex() const { return vertex; }

    void addInNeighbor(ESVertexData *in, const Arc *a);
    unsigned long long reparent(ESVertexData *in, const Arc *a);
    void findAndRemoveInNeighbor(ESVertexData *in, const Arc *a);

    bool isParent(ESVertexData *p);
    ESVertexData *getParentData() const;
    Vertex *getParent() const;

    bool checkIntegrity() const;

    std::vector<ESVertexData*> inNeighbors;
    unsigned long long parentIndex;
    unsigned long long level;

private:
    Vertex *vertex;
    FastPropertyMap<unsigned long long> *inNeighborIndices;
    std::vector<unsigned long long> recycledIndices;
};

std::ostream& operator<<(std::ostream& os, const ESVertexData *vd);

struct ES_Priority { unsigned long long operator()(const ESVertexData *vd) { return vd->getLevel(); }};

}

#endif // ESVERTEXDATA_H
