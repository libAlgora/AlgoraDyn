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

#ifndef DYNAMICDIGRAPH_H
#define DYNAMICDIGRAPH_H

//#include <vector>

namespace Algora {

class DiGraph;
class Vertex;

class DynamicDiGraph
{
public:
    explicit DynamicDiGraph();
    ~DynamicDiGraph();

    DiGraph *getDiGraph() const;
    unsigned long long getCurrentTime() const;
    unsigned long long getMaxTime() const;
    unsigned long long getNumberOfDeltas() const;
    unsigned long long getCurrentGraphSize() const;
    unsigned long long getCurrentArcSize() const;

    unsigned long long addVertex(unsigned long long timestamp);
    void addVertex(unsigned long long vertexId, unsigned long long timestamp);
    void removeVertex(unsigned long long vertexId, unsigned long long timestamp);
    void addArc(unsigned long long tailId, unsigned long long headId, unsigned long long timestamp, bool antedateVertexAdditions = false);
    void removeArc(unsigned long long tailId, unsigned long long headId, unsigned long long timestamp, bool removeIsolatedEnds = false);
    bool hasArc(unsigned long long tailId, unsigned long long headId);
    void clear();
    void compact(unsigned long long num);

    void resetToBigBang();
    bool applyNextOperation(bool sameTimestamp = false);
    bool applyNextDelta();
    bool lastOpWasVertexAddition() const;
    bool lastOpWasVertexRemoval() const;
    bool lastOpWasArcAddition() const;
    bool lastOpWasArcRemoval() const;
    bool lastOpWasMultiple() const;
    Vertex *getCurrentVertexForId(unsigned long long vertexId) const;
    unsigned long long idOfIthVertex(unsigned long long i) const;
    unsigned long long getSizeOfLastDelta() const;

    unsigned long long countVertexAdditions(unsigned long long timeFrom, unsigned long long timeUntil) const;
    unsigned long long countVertexRemovals(unsigned long long timeFrom, unsigned long long timeUntil) const;
    unsigned long long countArcAdditions(unsigned long long timeFrom, unsigned long long timeUntil) const;
    unsigned long long countArcRemovals(unsigned long long timeFrom, unsigned long long timeUntil) const;

    void squashTimes(unsigned long long timeFrom, unsigned long long timeUntil);
    void secondArcIsRemoval(bool sir);

private:
    struct CheshireCat;
    CheshireCat *grin;
};

}

#endif // DYNAMICDIGRAPH_H
