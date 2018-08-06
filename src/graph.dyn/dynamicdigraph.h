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

namespace Algora {

class DiGraph;
class Vertex;

class DynamicDiGraph
{
public:
    explicit DynamicDiGraph();
    ~DynamicDiGraph();

    DiGraph *getDiGraph() const;
    unsigned int getCurrentTime() const;
    unsigned int getMaxTime() const;
    unsigned int getNumberOfDeltas() const;
    unsigned int getCurrentGraphSize() const;
    unsigned int getCurrentArcSize() const;

    unsigned int addVertex(unsigned int timestamp);
    void addVertex(unsigned int vertexId, unsigned int timestamp);
    void removeVertex(unsigned int vertexId, unsigned int timestamp);
    void addArc(unsigned int tailId, unsigned int headId, unsigned int timestamp, bool antedateVertexAdditions = false);
    void removeArc(unsigned int tailId, unsigned int headId, unsigned int timestamp, bool removeIsolatedEnds = false);
    bool hasArc(unsigned int tailId, unsigned int headId);
    void clear();

    void resetToBigBang();
    bool applyNextOperation();
    bool applyNextDelta();
    Vertex *getCurrentVertexForId(unsigned int vertexId) const;
    unsigned int idOfIthVertex(unsigned int i) const;

    unsigned int countVertexAdditions(unsigned int timeFrom, unsigned int timeUntil) const;
    unsigned int countVertexRemovals(unsigned int timeFrom, unsigned int timeUntil) const;
    unsigned int countArcAdditions(unsigned int timeFrom, unsigned int timeUntil) const;
    unsigned int countArcRemovals(unsigned int timeFrom, unsigned int timeUntil) const;

    void squashTimes(unsigned int timeFrom, unsigned int timeUntil);

private:
    class CheshireCat;
    CheshireCat *grin;
};

}

#endif // DYNAMICDIGRAPH_H
