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

#ifndef DYNAMICDIGRAPH_H
#define DYNAMICDIGRAPH_H

#include <vector>
#include "graph.incidencelist/incidencelistgraph.h"

namespace Algora {

struct Operation;
struct AddArcOperation;

class DynamicDiGraph
{
public:
    typedef unsigned long long VertexIdentifier;
    typedef unsigned long long DynamicTime;
    typedef std::vector<DynamicTime>::size_type size_type;

    explicit DynamicDiGraph();
    virtual ~DynamicDiGraph();

    IncidenceListGraph *getDiGraph() const;
    DynamicTime getCurrentTime() const;
    DynamicTime getTimeOfXthNextDelta(DynamicTime x, bool forward) const;
    DynamicTime getMaxTime() const;
    const std::vector<DynamicTime> &getTimestamps() const;
    size_type getNumberOfDeltas() const;
    DiGraph::size_type getConstructedGraphSize() const;
    DiGraph::size_type getConstructedArcSize() const;
    DiGraph::size_type getMinVertexId() const;
    DiGraph::size_type getMaxVertexId() const;

    VertexIdentifier addVertex(DynamicTime timestamp);
    void addVertex(VertexIdentifier vertexId, DynamicTime timestamp);
    void removeVertex(VertexIdentifier vertexId, DynamicTime timestamp);
    void addArc(VertexIdentifier tailId, VertexIdentifier headId,
                DynamicTime timestamp, bool antedateVertexAdditions = false);
    void addArcAndRemoveIn(VertexIdentifier tailId, VertexIdentifier headId,
                DynamicTime timestamp, size_type ageInDeltas = 0,
                bool antedateVertexAdditions = false);
    void removeArc(VertexIdentifier tailId, VertexIdentifier headId,
                   DynamicTime timestamp);
    void removeArc(VertexIdentifier tailId, VertexIdentifier headId,
                   DynamicTime timestamp, bool removeIsolatedEnds);
    void noop(DynamicTime timestamp);
    bool hasArc(VertexIdentifier tailId, VertexIdentifier headId);
    void clear();
    void compact(size_type num);

    void resetToBigBang();
    bool applyNextOperation(bool sameTimestamp = false);
    bool applyNextDelta();
    bool lastOpWasVertexAddition() const;
    bool lastOpWasVertexRemoval() const;
    bool lastOpWasArcAddition() const;
    bool lastOpWasArcRemoval() const;
    bool lastOpWasMultiple() const;
    bool lastOpWasNoop() const;
    Vertex *getCurrentVertexForId(VertexIdentifier vertexId) const;
    VertexIdentifier idOfIthVertex(size_type i);
    size_type getSizeOfLastDelta() const;
    size_type getSizeOfFinalDelta() const;

    size_type countVertexAdditions(DynamicTime timeFrom, DynamicTime timeUntil) const;
    size_type countVertexRemovals(DynamicTime timeFrom, DynamicTime timeUntil) const;
    size_type countArcAdditions(DynamicTime timeFrom, DynamicTime timeUntil) const;
    size_type countArcRemovals(DynamicTime timeFrom, DynamicTime timeUntil) const;
    size_type countNoops(DynamicTime timeFrom, DynamicTime timeUntil) const;

    void squashTimes(DynamicTime timeFrom, DynamicTime timeUntil);
    void secondArcIsRemoval(bool sir);

    void setDefaultArcAge(size_type defaultAge);
    size_type getDefaultArcAge() const;

    void setRemoveIsolatedEnds(bool remove);
    bool removeIsolatedEnds() const;

protected:
    void addOperation(DynamicTime timestamp, Operation *op);
    Operation *getLastOperation() const;
    void replaceLastOperation(Operation *op);
    AddArcOperation *findAddArcOperation(VertexIdentifier tailId, VertexIdentifier headId);
    void removeArc(AddArcOperation *aao);

private:
    struct CheshireCat;
    CheshireCat *grin;
};

}

#endif // DYNAMICDIGRAPH_H
