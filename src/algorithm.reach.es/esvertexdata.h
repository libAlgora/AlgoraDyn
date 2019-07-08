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
#include <limits>
#include <iostream>
#include <property/fastpropertymap.h>
#include <cassert>

#include "graph/digraph.h"

namespace Algora {

class DynamicDiGraphAlgorithm;
class Vertex;
class Arc;

class ESVertexData
{
    friend std::ostream& operator<<(std::ostream &os, const ESVertexData *vd);

public:
    typedef DiGraph::size_type level_type;
    static constexpr level_type UNREACHABLE = std::numeric_limits<level_type>::max();

    ESVertexData(FastPropertyMap<level_type> *inNeighborIndices,
            Vertex *v, ESVertexData *p = nullptr, Arc *a = nullptr, level_type l = UNREACHABLE);

    void reset(ESVertexData *p = nullptr, Arc *a = nullptr, level_type l = UNREACHABLE);

    void setUnreachable();
    bool isReachable() const;
    level_type getLevel() const {
        return isReachable() ? level : UNREACHABLE;
    }
    Vertex *getVertex() const { return vertex; }
    Arc *getTreeArc() const;

    void addInNeighbor(ESVertexData *in, Arc *a);
    level_type reparent(ESVertexData *in, Arc *a);
    void findAndRemoveInNeighbor(ESVertexData *in, const Arc *a);

    bool isParent(const ESVertexData *p);
    bool isTreeArc(const Arc *a);
    ESVertexData *getParentData() const;
    Vertex *getParent() const;

    bool checkIntegrity() const;

    std::vector<ESVertexData*> inNeighbors;
    std::vector<Arc*> inArcs;
    DiGraph::size_type parentIndex;
    level_type level;

private:
    Vertex *vertex;
    FastPropertyMap<DiGraph::size_type> *inNeighborIndices;
    std::vector<DiGraph::size_type> recycledIndices;
};

std::ostream& operator<<(std::ostream& os, const ESVertexData *vd);

struct ES_Priority { ESVertexData::level_type operator()(const ESVertexData *vd) { return vd->getLevel(); }};

}

#endif // ESVERTEXDATA_H
