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

#include "dynamicdigraph.h"

#include "graph.incidencelist/incidencelistgraph.h"
#include "graph.incidencelist/incidencelistvertex.h"

#include <vector>
#include <unordered_map>

#include <cassert>

//#define DEBUG_DYNDIGRAPH

#ifdef DEBUG_DYNDIGRAPH
#include <iostream>
#define PRINT_DEBUG(msg) std::cout << msg << std::endl;
#define IF_DEBUG(cmd) cmd;
#else
#define PRINT_DEBUG(msg)
#define IF_DEBUG(cmd)
#endif

namespace Algora {

struct Operation {
    enum Type { VERTEX_ADDITION, VERTEX_REMOVAL, ARC_ADDITION, ARC_REMOVAL, MULTIPLE };
    virtual ~Operation() {}
    virtual void apply(IncidenceListGraph *graph) = 0;
    virtual Type getType() const = 0;
    virtual void reset() { }
};

struct OperationSet : public Operation {
    std::vector<Operation*> operations;
    virtual ~OperationSet() {
        for (auto op : operations) {
            delete op;
        }
    }

    virtual void apply(IncidenceListGraph *graph) {
        for (auto op : operations) {
            op->apply(graph);
        }
    }
    virtual Type getType() const override { return MULTIPLE; }
    virtual void reset() override {
        for (auto op : operations) {
            op->reset();
        }
    }
};

struct AddVertexOperation : public Operation {
    Vertex *vertex;
    Vertex *constructionVertex;
    unsigned int vertexId;

    AddVertexOperation(Vertex *cv, unsigned int vId) : vertex(nullptr), constructionVertex(cv), vertexId(vId) { }

    virtual void apply(IncidenceListGraph *graph) {
        vertex = graph->addVertex();
        vertex->setName(std::to_string(vertexId));
    }
    virtual Type getType() const override { return VERTEX_ADDITION; }
    virtual void reset() override { vertex = nullptr; }
};

struct RemoveVertexOperation : public Operation {
    AddVertexOperation *addOp;

    RemoveVertexOperation(AddVertexOperation *avo) : addOp(avo) {}

    virtual void apply(IncidenceListGraph *graph) {
        graph->removeVertex(addOp->vertex);
    }
    virtual Type getType() const override { return VERTEX_REMOVAL; }
};

struct AddArcOperation : public Operation {
    AddVertexOperation *tail;
    AddVertexOperation *head;
    Arc *arc;
    Arc *constructionArc;

    AddArcOperation(AddVertexOperation *t, AddVertexOperation *h, Arc *ca)
        : tail(t), head(h), arc(nullptr), constructionArc(ca) { }

    virtual void apply(IncidenceListGraph *graph) {
        arc = graph->addArc(tail->vertex, head->vertex);
    }
    virtual Type getType() const override { return ARC_ADDITION; }
    virtual void reset() override { arc = nullptr; }
};

struct RemoveArcOperation : public Operation {
    AddArcOperation *addOp;

    RemoveArcOperation(AddArcOperation *aao) : addOp(aao) { }

    virtual void apply(IncidenceListGraph *graph) {
        graph->removeArc(addOp->arc);
    }
    virtual Type getType() const override { return ARC_REMOVAL; }
};

struct DynamicDiGraph::CheshireCat {
    IncidenceListGraph dynGraph;
    IncidenceListGraph constructionGraph;

    std::vector<unsigned int> timestamps;
    std::vector<Operation*> operations;
    std::vector<unsigned int> offset;
    OperationSet antedated;

    unsigned int timeIndex;
    unsigned int opIndex;
    unsigned int numVertices;

    std::vector<AddVertexOperation*> vertices;
    std::unordered_map<Arc*,AddArcOperation*> constructionArcMap;

    bool doubleArcIsRemoval;

    CheshireCat() : timeIndex(0U), opIndex(0U), doubleArcIsRemoval(false) { clear(); }
    ~CheshireCat() {
        clear();
    }

    void reset() {
        timeIndex = 0U;
        opIndex = 0U;
        dynGraph.clear();

        for (auto op : operations) {
            op->reset();
        }
    }

    void init() {
        if (!antedated.operations.empty()) {
            antedated.apply(&dynGraph);
        }
    }

    void clear() {
        reset();
        vertices.clear();
        constructionArcMap.clear();
        constructionGraph.clear();
        timestamps.clear();

        for (auto op : operations) {
            delete op;
        }
        operations.clear();
        antedated.operations.clear();
        offset.clear();
    }

    void checkTimestamp(unsigned int timestamp) {
        if (!timestamps.empty() && timestamp < timestamps.back()) {
            throw std::invalid_argument("Timestamps must be non-decreasing.");
        }
        extendTime(timestamp);
    }

    void extendTime(unsigned int timestamp) {
        if (timestamps.empty() || timestamps.back() < timestamp) {
            PRINT_DEBUG( "Extending time from " << (timestamps.empty() ? 0U : timestamps.back()) << " to " << timestamp )
            timestamps.push_back(timestamp);
            offset.push_back(operations.size());
        }
    }

    unsigned int addVertex(unsigned int timestamp, bool atEnd, unsigned int vertexId = 0U, bool okIfExists = false) {
        checkTimestamp(timestamp);

        if (atEnd) {
            vertexId = vertices.size();
        }
        if (vertexId >= vertices.size()) {
            vertices.resize(vertexId + 1, nullptr);
        } else if (!okIfExists && vertexId < vertices.size() && vertices.at(vertexId) != nullptr) {
            throw std::invalid_argument("A vertex with this id exists already.");
        }

        Vertex *cv = constructionGraph.addVertex();
        AddVertexOperation *avo = new AddVertexOperation(cv, vertexId);
        operations.push_back(avo);
        vertices[vertexId] = avo;

        return vertexId;
    }

    void removeVertex(unsigned int vertexId, unsigned int timestamp) {
        if (vertexId >= vertices.size() || vertices.at(vertexId) == nullptr) {
            throw std::invalid_argument("Vertex ID does not exist.");
        }

        checkTimestamp(timestamp);

        AddVertexOperation *avo = vertices[vertexId];

        auto removeIncidentArcs = [&](Arc *a) {
            constructionArcMap.erase(a);
        };

        constructionGraph.mapOutgoingArcs(avo->constructionVertex, removeIncidentArcs);
        constructionGraph.mapIncomingArcs(avo->constructionVertex, removeIncidentArcs);

        constructionGraph.removeVertex(avo->constructionVertex);
        RemoveVertexOperation *rvo = new RemoveVertexOperation(avo);
        operations.push_back(rvo);
        vertices[vertexId] = nullptr;
    }

    void addArc(unsigned int tailId, unsigned int headId, unsigned int timestamp, bool antedateVertexAddition)
    {
        checkTimestamp(timestamp);

        unsigned int maxId = tailId > headId ? tailId : headId;
        if (maxId >= vertices.size()) {
            vertices.resize(maxId + 1, nullptr);
        }

        AddVertexOperation *avoTail = vertices.at(tailId);;
        AddVertexOperation *avoHead = vertices.at(headId);

        OperationSet *os = nullptr;
        if (avoTail == nullptr || avoHead == nullptr) {
            if (antedateVertexAddition && timeIndex == 0U) {
                os = &antedated;
            } else {
                os = new OperationSet;
            }

            if (avoTail == nullptr) {
                Vertex *cv = constructionGraph.addVertex();
                avoTail = new AddVertexOperation(cv, tailId);
                os->operations.push_back(avoTail);
                vertices[tailId] = avoTail;

                if (tailId == headId) {
                    avoHead = avoTail;
                }
            }
            if (avoHead == nullptr) {
                Vertex *cv = constructionGraph.addVertex();
                avoHead = new AddVertexOperation(cv, headId);
                os->operations.push_back(avoHead);
                vertices[headId] = avoHead;
            }
        }


        Arc *ca = constructionGraph.addArc(avoTail->constructionVertex, avoHead->constructionVertex);
        AddArcOperation *aao = new AddArcOperation(avoTail, avoHead, ca);
        if (os && os != &antedated) {
            os->operations.push_back(aao);
            operations.push_back(os);
        } else {
          operations.push_back(aao);
        }
        constructionArcMap[ca] = aao;
    }

    Arc *findArc(unsigned int tailId, unsigned int headId) {
        if (tailId >= vertices.size() || headId >= vertices.size()) {
            return nullptr;
        }
        auto avoTail = vertices[tailId];
        auto avoHead = vertices[headId];
        if (avoTail == nullptr || avoHead == nullptr) {
            return nullptr;
        }
        Vertex *ct = avoTail->constructionVertex;
        Vertex *ch = avoHead->constructionVertex;
        Arc *ca = nullptr;
        constructionGraph.mapOutgoingArcsUntil(ct, [&](Arc *a) {
            if (a->getHead() == ch) {
                ca = a;
            }
        }, [&](const Arc*) { return ca != nullptr; });

        return ca;
    }

    void removeArc(unsigned int tailId, unsigned int headId, unsigned int timestamp, bool removeIsolatedEnds) {
        Arc *ca = findArc(tailId, headId);
        if (!ca) {
            throw std::invalid_argument("Arc does not exist.");
        }

        checkTimestamp(timestamp);

        AddArcOperation *aao = constructionArcMap[ca];
        assert(ca == aao->constructionArc);
        RemoveArcOperation *rao = new RemoveArcOperation(aao);
        constructionGraph.removeArc(ca);
        constructionArcMap.erase(ca);

        if (removeIsolatedEnds) {
            AddVertexOperation *avoTail = vertices[tailId];
            AddVertexOperation *avoHead = vertices[headId];
            IncidenceListVertex *tail = dynamic_cast<IncidenceListVertex*>(avoTail->constructionVertex);
            IncidenceListVertex *head = dynamic_cast<IncidenceListVertex*>(avoHead->constructionVertex);
            if (tail->isIsolated() || head->isIsolated()) {
                OperationSet *op = new OperationSet;
                op->operations.push_back(rao);
                if (tail->isIsolated()) {
                    constructionGraph.removeVertex(avoTail->constructionVertex);
                    RemoveVertexOperation *rvo = new RemoveVertexOperation(avoTail);
                    op->operations.push_back(rvo);
                    vertices[tailId] = nullptr;
                }
                if (tailId != headId && head->isIsolated()) {
                    constructionGraph.removeVertex(avoHead->constructionVertex);
                    RemoveVertexOperation *rvo = new RemoveVertexOperation(avoHead);
                    op->operations.push_back(rvo);
                    vertices[headId] = nullptr;
                }
                operations.push_back(op);
            } else {
                operations.push_back(rao);
            }
        } else {
            operations.push_back(rao);
        }
    }

    bool advance(bool sameTime = false) {
        if (opIndex >= operations.size()) {
            PRINT_DEBUG("Cannot advance further.")
            return false;
        }

        if (timeIndex + 1 < timestamps.size() && opIndex == offset[timeIndex + 1]) {
            if (sameTime) {
                return false;
            }
            timeIndex++;
        }

        if (opIndex == 0) {
            init();
        }

        return true;
    }

    bool nextOp(bool sameTime = false) {
        if (!advance(sameTime)) {
            return false;
        }
        operations[opIndex]->apply(&dynGraph);
        opIndex++;
        return true;
    }

    bool nextDelta() {
        if (!advance()) {
            return false;
        }

        unsigned int maxOp = operations.size();
        if (timeIndex + 1 < timestamps.size()) {
            maxOp = offset[timeIndex + 1];
        }
        PRINT_DEBUG( "Applying delta #op" << opIndex << " - #op" << maxOp)
        while (opIndex < maxOp) {
            operations[opIndex]->apply(&dynGraph);
            opIndex++;
        }
        return true;
    }

    unsigned int findTimeIndex(unsigned int timestamp) const {
        unsigned int tIndex = 0U;
        while (tIndex < timestamps.size() && timestamps[tIndex] < timestamp) {
            tIndex++;
        }
        return tIndex;
    }

    bool lastOpHadType(Operation::Type type) const {
        if (opIndex == 0U) {
            return false;
        }
        auto op = operations[opIndex - 1];
        if (op->getType() == type) {
            return true;
        } else if (op->getType() == Operation::Type::MULTIPLE) {
            OperationSet *os = dynamic_cast<OperationSet*>(op);
            assert(os);
            return os->operations.back()->getType() == type;
        }
        return false;
    }

    unsigned int countOperations(unsigned int timeFrom, unsigned int timeUntil, Operation::Type type) const {
        unsigned int tIndexFrom = findTimeIndex(timeFrom);
        if (tIndexFrom >= timestamps.size()) {
                return 0U;
        }
        unsigned int tIndexUntil = findTimeIndex(timeUntil) + 1;
        if (tIndexUntil <= tIndexFrom) {
            return 0U;
        }
        unsigned int opIndexMax = tIndexUntil < offset.size() ? offset[tIndexUntil] : operations.size();
        unsigned int numOperations = 0U;
        for (unsigned int opI = offset[tIndexFrom]; opI < opIndexMax; opI++) {
            Operation *op = operations[opI];
            if (op->getType() == type) {
                numOperations++;
            } else if (op->getType() == Operation::Type::MULTIPLE) {
                // there should be no nested operation sets...
                OperationSet *os = dynamic_cast<OperationSet*>(op);
                if (os) {
                    for (Operation *o : os->operations) {
                        if (o->getType() == type) {
                            numOperations++;
                        }
                    }
                }
            }
        }
        return numOperations;
    }

    void squashTimes(unsigned int timeFrom, unsigned int timeUntil) {
        unsigned int squashOn = findTimeIndex(timeFrom);
        unsigned int squashMax = findTimeIndex(timeUntil) + 1;
        timestamps.erase(timestamps.cbegin() + squashOn + 1, timestamps.cbegin() + squashMax);
        offset.erase(offset.cbegin() + squashOn + 1, offset.cbegin() + squashMax);
    }

    Vertex *vertexForId(unsigned int vertexId) const {
        if (vertexId >= vertices.size() || vertices.at(vertexId) == nullptr) {
            return nullptr;
        }
        return vertices.at(vertexId)->vertex;
    }
};

DynamicDiGraph::DynamicDiGraph()
    : grin(new CheshireCat)
{

}

DynamicDiGraph::~DynamicDiGraph()
{
    delete grin;
}

DiGraph *DynamicDiGraph::getDiGraph() const
{
    return &(grin->dynGraph);
}

unsigned int DynamicDiGraph::getCurrentTime() const
{
    return grin->timestamps.empty() || (grin->timeIndex == 0U && grin->opIndex == 0U)
            ? 0U : grin->timestamps[grin->timeIndex];
}

unsigned int DynamicDiGraph::getMaxTime() const
{
    return grin->timestamps.empty() ? 0U : grin->timestamps.back();
}

unsigned int DynamicDiGraph::getNumberOfDeltas() const
{
    return grin->timestamps.size();
}

unsigned int DynamicDiGraph::getCurrentGraphSize() const
{
    return grin->constructionGraph.getSize();
}

unsigned int DynamicDiGraph::getCurrentArcSize() const
{
    return grin->constructionArcMap.size();
}

unsigned int DynamicDiGraph::addVertex(unsigned int timestamp)
{
    return grin->addVertex(timestamp, true);
}

void DynamicDiGraph::addVertex(unsigned int vertexId, unsigned int timestamp)
{
    grin->addVertex(timestamp, false, vertexId);
}

void DynamicDiGraph::removeVertex(unsigned int vertexId, unsigned int timestamp)
{
    grin->removeVertex(vertexId, timestamp);
}

void DynamicDiGraph::addArc(unsigned int tailId, unsigned int headId, unsigned int timestamp, bool antedateVertexAdditions)
{
    if (grin->doubleArcIsRemoval && hasArc(tailId, headId)) {
        grin->removeArc(tailId, headId, timestamp, antedateVertexAdditions);
    } else {
        grin->addArc(tailId, headId, timestamp, antedateVertexAdditions);
    }
}

void DynamicDiGraph::removeArc(unsigned int tailId, unsigned int headId, unsigned int timestamp, bool removeIsolatedEnds)
{
    grin->removeArc(tailId, headId, timestamp, removeIsolatedEnds);
}

bool DynamicDiGraph::hasArc(unsigned int tailId, unsigned int headId)
{
    return grin->findArc(tailId, headId) != nullptr;
}

void DynamicDiGraph::clear()
{
    grin->clear();
}

void DynamicDiGraph::resetToBigBang()
{
    grin->reset();
}

bool DynamicDiGraph::applyNextOperation(bool sameTimestamp)
{
    return grin->nextOp(sameTimestamp);
}

bool DynamicDiGraph::applyNextDelta()
{
    return grin->nextDelta();
}

bool DynamicDiGraph::lastOpWasVertexAddition() const
{
    return grin->lastOpHadType(Operation::Type::VERTEX_ADDITION);
}

bool DynamicDiGraph::lastOpWasVertexRemoval() const
{
    return grin->lastOpHadType(Operation::Type::VERTEX_REMOVAL);
}

bool DynamicDiGraph::lastOpWasArcAddition() const
{
    return grin->lastOpHadType(Operation::Type::ARC_ADDITION);
}

bool DynamicDiGraph::lastOpWasArcRemoval() const
{
    return grin->lastOpHadType(Operation::Type::ARC_REMOVAL);
}

bool DynamicDiGraph::lastOpWasMultiple() const
{
    return grin->lastOpHadType(Operation::Type::MULTIPLE);
}

Vertex *DynamicDiGraph::getCurrentVertexForId(unsigned int vertexId) const
{
    return grin->vertexForId(vertexId);
}

unsigned int DynamicDiGraph::idOfIthVertex(unsigned int i) const
{
    return stoul(grin->dynGraph.vertexAt(i)->getName());
}

unsigned int DynamicDiGraph::countVertexAdditions(unsigned int timeFrom, unsigned int timeUntil) const
{
    return grin->countOperations(timeFrom, timeUntil, Operation::Type::VERTEX_ADDITION);
}

unsigned int DynamicDiGraph::countVertexRemovals(unsigned int timeFrom, unsigned int timeUntil) const
{
    return grin->countOperations(timeFrom, timeUntil, Operation::Type::VERTEX_REMOVAL);
}

unsigned int DynamicDiGraph::countArcAdditions(unsigned int timeFrom, unsigned int timeUntil) const
{
    return grin->countOperations(timeFrom, timeUntil, Operation::Type::ARC_ADDITION);
}

unsigned int DynamicDiGraph::countArcRemovals(unsigned int timeFrom, unsigned int timeUntil) const
{
    return grin->countOperations(timeFrom, timeUntil, Operation::Type::ARC_REMOVAL);
}

void DynamicDiGraph::squashTimes(unsigned int timeFrom, unsigned int timeUntil)
{
    grin->reset();
    grin->squashTimes(timeFrom, timeUntil);
}

void DynamicDiGraph::secondArcIsRemoval(bool sir)
{
    grin->doubleArcIsRemoval = sir;
}

}
