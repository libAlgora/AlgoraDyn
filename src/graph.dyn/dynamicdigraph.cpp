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
    Type type = MULTIPLE;

    virtual ~OperationSet() override {
        for (auto op : operations) {
            delete op;
        }
    }

    virtual void apply(IncidenceListGraph *graph) override {
        for (auto op : operations) {
            op->apply(graph);
        }
    }
    virtual Type getType() const override { return type; }
    virtual void reset() override {
        for (auto op : operations) {
            op->reset();
        }
    }
};

struct AddVertexOperation : public Operation {
    Vertex *vertex;
    Vertex *constructionVertex;
    unsigned long long vertexId;

    AddVertexOperation(Vertex *cv, unsigned long long vId) : vertex(nullptr), constructionVertex(cv), vertexId(vId) { }

    virtual void apply(IncidenceListGraph *graph) override {
        vertex = graph->addVertex();
        vertex->setName(std::to_string(vertexId));
    }
    virtual Type getType() const override { return VERTEX_ADDITION; }
    virtual void reset() override { vertex = nullptr; }
};

struct RemoveVertexOperation : public Operation {
    AddVertexOperation *addOp;

    RemoveVertexOperation(AddVertexOperation *avo) : addOp(avo) {}

    virtual void apply(IncidenceListGraph *graph) override {
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

    virtual void apply(IncidenceListGraph *graph) override {
        arc = graph->addArc(tail->vertex, head->vertex);
    }
    virtual Type getType() const override { return ARC_ADDITION; }
    virtual void reset() override { arc = nullptr; }
};

struct RemoveArcOperation : public Operation {
    AddArcOperation *addOp;

    RemoveArcOperation(AddArcOperation *aao) : addOp(aao) { }

    virtual void apply(IncidenceListGraph *graph) override {
        graph->removeArc(addOp->arc);
    }
    virtual Type getType() const override { return ARC_REMOVAL; }
};

struct DynamicDiGraph::CheshireCat {
    IncidenceListGraph dynGraph;
    IncidenceListGraph constructionGraph;

    std::vector<unsigned long long> timestamps;
    std::vector<Operation*> operations;
    std::vector<unsigned long long> offset;
    OperationSet antedated;

    unsigned long long timeIndex;
    unsigned long long opIndex;
    //unsigned long long numVertices;

    bool doubleArcIsRemoval;

    std::vector<AddVertexOperation*> vertices;
    std::unordered_map<Arc*,AddArcOperation*> constructionArcMap;

    std::unordered_map<Vertex*,unsigned long long> vertexToIdMap;
    unsigned long long vertexToIdMapNextOpIndex;

    unsigned long numResets;
    unsigned long long curVertexSize;
    unsigned long long curArcSize;
    unsigned long long maxVertexSize;
    unsigned long long maxArcSize;

    CheshireCat() : timeIndex(0U), opIndex(0U), doubleArcIsRemoval(false),
        vertexToIdMapNextOpIndex(0ULL), numResets(0U),
        curVertexSize(0ULL), curArcSize(0ULL), maxVertexSize(0ULL), maxArcSize(0ULL)
        { clear(); }
    ~CheshireCat() {
        clear();
    }

    void reset() {
        timeIndex = 0U;
        opIndex = 0U;
        if (numResets == 2U) {
            //dynGraph.clearAndRelease();
            //dynGraph.reserveVertexCapacity(maxVertexSize);
            //dynGraph.reserveArcCapacity(maxArcSize);
            std::vector<Vertex*> vertices;
            vertices.reserve(maxVertexSize);
            std::vector<Arc*> arcs;
            arcs.reserve(maxArcSize);
            dynGraph.clear();
            for (auto i = 0UL; i < maxVertexSize; i++) {
                vertices.push_back(dynGraph.addVertex());
            }
            for (auto i = 0UL; i < maxArcSize; i++) {
                arcs.push_back(dynGraph.addArc(vertices.at(0), vertices.at(1)));
            }
            for (auto a : arcs) {
                dynGraph.removeArc(a);
            }
            for (auto v : vertices) {
                dynGraph.removeVertex(v);
            }
        } else {
            dynGraph.clear();
        }
        vertexToIdMap.clear();
        vertexToIdMapNextOpIndex = 0ULL;


        for (auto op : operations) {
            op->reset();
        }
        numResets++;
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
        curVertexSize = 0ULL;
        curArcSize = 0ULL;
        maxVertexSize = 0ULL;
        maxArcSize = 0ULL;

        for (auto op : operations) {
            delete op;
        }
        operations.clear();
        antedated.operations.clear();
        offset.clear();
    }

    void checkTimestamp(unsigned long long timestamp) {
        if (!timestamps.empty() && timestamp < timestamps.back()) {
            throw std::invalid_argument("Timestamps must be non-decreasing.");
        }
        extendTime(timestamp);
    }

    void extendTime(unsigned long long timestamp) {
        if (timestamps.empty() || timestamps.back() < timestamp) {
            PRINT_DEBUG( "Extending time from " << (timestamps.empty() ? 0U : timestamps.back()) << " to " << timestamp )
            timestamps.push_back(timestamp);
            offset.push_back(operations.size());
        }
    }

    unsigned long long addVertex(unsigned long long timestamp, bool atEnd, unsigned long long vertexId = 0U, bool okIfExists = false) {
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

        curVertexSize++;
        if (curVertexSize > maxVertexSize) {
            maxVertexSize = curVertexSize;
        }

        return vertexId;
    }

    void removeVertex(unsigned long long vertexId, unsigned long long timestamp) {
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

        curVertexSize--;
    }

    void addArc(unsigned long long tailId, unsigned long long headId, unsigned long long timestamp, bool antedateVertexAddition)
    {
        checkTimestamp(timestamp);

        unsigned long long maxId = tailId > headId ? tailId : headId;
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
                curVertexSize++;

                if (tailId == headId) {
                    avoHead = avoTail;
                }
            }
            if (avoHead == nullptr) {
                Vertex *cv = constructionGraph.addVertex();
                avoHead = new AddVertexOperation(cv, headId);
                os->operations.push_back(avoHead);
                vertices[headId] = avoHead;
                curVertexSize++;
            }

            if (curVertexSize > maxVertexSize) {
                maxVertexSize = curVertexSize;
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

        curArcSize++;
        if (curArcSize > maxArcSize) {
            maxArcSize = curArcSize;
        }
    }

    Arc *findArc(unsigned long long tailId, unsigned long long headId) {
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

    void removeArc(unsigned long long tailId, unsigned long long headId, unsigned long long timestamp, bool removeIsolatedEnds) {
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
                    curVertexSize--;
                }
                if (tailId != headId && head->isIsolated()) {
                    constructionGraph.removeVertex(avoHead->constructionVertex);
                    RemoveVertexOperation *rvo = new RemoveVertexOperation(avoHead);
                    op->operations.push_back(rvo);
                    vertices[headId] = nullptr;
                    curVertexSize--;
                }
                operations.push_back(op);
            } else {
                operations.push_back(rao);
            }
        } else {
            operations.push_back(rao);
        }
        curArcSize--;
    }

    void compact(unsigned long long num) {
        std::vector<Operation*> ops;
        for (auto i = 0U; i < num; i++) {
            auto *last = operations.back();
            operations.pop_back();
            auto nestedOs = dynamic_cast<OperationSet*>(last);
            if (nestedOs) {
                while (!nestedOs->operations.empty()) {
                    ops.push_back(nestedOs->operations.back());
                    nestedOs->operations.pop_back();
                }
                delete nestedOs;
            } else {
                ops.push_back(last);
            }
        }
        std::reverse(std::begin(ops), std::end(ops));
        auto *os = new OperationSet;
        os->operations = std::move(ops);
        operations.push_back(os);
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

        unsigned long long maxOp = operations.size();
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

    unsigned long long findTimeIndex(unsigned long long timestamp) const {
        unsigned long long tIndex = 0U;
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

    unsigned long long countOperations(unsigned long long timeFrom, unsigned long long timeUntil, Operation::Type type) const {
        unsigned long long tIndexFrom = findTimeIndex(timeFrom);
        if (tIndexFrom >= timestamps.size()) {
                return 0U;
        }
        unsigned long long tIndexUntil = findTimeIndex(timeUntil) + 1;
        if (tIndexUntil <= tIndexFrom) {
            return 0U;
        }
        unsigned long long opIndexMax = tIndexUntil < offset.size() ? offset[tIndexUntil] : operations.size();
        unsigned long long numOperations = 0U;
        for (unsigned long long opI = offset[tIndexFrom]; opI < opIndexMax; opI++) {
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

    void squashTimes(unsigned long long timeFrom, unsigned long long timeUntil) {
        unsigned long long squashOn = findTimeIndex(timeFrom);
        unsigned long long squashMax = findTimeIndex(timeUntil) + 1;
        timestamps.erase(timestamps.cbegin() + squashOn + 1, timestamps.cbegin() + squashMax);
        offset.erase(offset.cbegin() + squashOn + 1, offset.cbegin() + squashMax);
    }

    Vertex *vertexForId(unsigned long long vertexId) const {
        if (vertexId >= vertices.size() || vertices.at(vertexId) == nullptr) {
            return nullptr;
        }
        return vertices.at(vertexId)->vertex;
    }

    unsigned long long getSizeOfLastDelta() {
        if (timeIndex + 1U == offset.size()) {
            return operations.size() - offset[timeIndex];
        }
        return offset[timeIndex + 1U] - offset[timeIndex];
    }

    unsigned long long idOfIthVertex(unsigned long long i) {
        for (;vertexToIdMapNextOpIndex < opIndex; vertexToIdMapNextOpIndex++) {
            Operation *op = operations[vertexToIdMapNextOpIndex];
            if (op->getType() == Operation::Type::VERTEX_ADDITION) {
                AddVertexOperation *avo = dynamic_cast<AddVertexOperation*>(op);
                vertexToIdMap[avo->vertex] = avo->vertexId;
            } else  if (op->getType() == Operation::Type::VERTEX_REMOVAL) {
                RemoveVertexOperation *rvo = dynamic_cast<RemoveVertexOperation*>(op);
                vertexToIdMap.erase(rvo->addOp->vertex);
            }
        }
        return vertexToIdMap.at(dynGraph.vertexAt(i));
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

unsigned long long DynamicDiGraph::getCurrentTime() const
{
    return grin->timestamps.empty() || (grin->timeIndex == 0U && grin->opIndex == 0U)
            ? 0U : grin->timestamps[grin->timeIndex];
}

unsigned long long DynamicDiGraph::getMaxTime() const
{
    return grin->timestamps.empty() ? 0U : grin->timestamps.back();
}

unsigned long long DynamicDiGraph::getNumberOfDeltas() const
{
    return grin->timestamps.size();
}

unsigned long long DynamicDiGraph::getCurrentGraphSize() const
{
    return grin->constructionGraph.getSize();
}

unsigned long long DynamicDiGraph::getCurrentArcSize() const
{
    return grin->constructionArcMap.size();
}

unsigned long long DynamicDiGraph::addVertex(unsigned long long timestamp)
{
    return grin->addVertex(timestamp, true);
}

void DynamicDiGraph::addVertex(unsigned long long vertexId, unsigned long long timestamp)
{
    grin->addVertex(timestamp, false, vertexId);
}

void DynamicDiGraph::removeVertex(unsigned long long vertexId, unsigned long long timestamp)
{
    grin->removeVertex(vertexId, timestamp);
}

void DynamicDiGraph::addArc(unsigned long long tailId, unsigned long long headId, unsigned long long timestamp, bool antedateVertexAdditions)
{
    if (grin->doubleArcIsRemoval && hasArc(tailId, headId)) {
        grin->removeArc(tailId, headId, timestamp, antedateVertexAdditions);
    } else {
        grin->addArc(tailId, headId, timestamp, antedateVertexAdditions);
    }
}

void DynamicDiGraph::removeArc(unsigned long long tailId, unsigned long long headId, unsigned long long timestamp, bool removeIsolatedEnds)
{
    grin->removeArc(tailId, headId, timestamp, removeIsolatedEnds);
}

bool DynamicDiGraph::hasArc(unsigned long long tailId, unsigned long long headId)
{
    return grin->findArc(tailId, headId) != nullptr;
}

void DynamicDiGraph::clear()
{
    grin->clear();
}

void DynamicDiGraph::compact(unsigned long long num)
{
    if (num < 1U) {
        throw std::invalid_argument("Can only compact positive number of operations.");
    } else if (num > grin->operations.size() - grin->opIndex) {
        throw std::invalid_argument("Cannot compact already executed operations.");
    }
    grin->compact(num);
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

Vertex *DynamicDiGraph::getCurrentVertexForId(unsigned long long vertexId) const
{
    return grin->vertexForId(vertexId);
}

unsigned long long DynamicDiGraph::idOfIthVertex(unsigned long long i)
{
    //return stoull(grin->dynGraph.vertexAt(i)->getName());
    return grin->idOfIthVertex(i);
}

unsigned long long DynamicDiGraph::getSizeOfLastDelta() const
{
    return grin->getSizeOfLastDelta();
}

unsigned long long DynamicDiGraph::countVertexAdditions(unsigned long long timeFrom, unsigned long long timeUntil) const
{
    return grin->countOperations(timeFrom, timeUntil, Operation::Type::VERTEX_ADDITION);
}

unsigned long long DynamicDiGraph::countVertexRemovals(unsigned long long timeFrom, unsigned long long timeUntil) const
{
    return grin->countOperations(timeFrom, timeUntil, Operation::Type::VERTEX_REMOVAL);
}

unsigned long long DynamicDiGraph::countArcAdditions(unsigned long long timeFrom, unsigned long long timeUntil) const
{
    return grin->countOperations(timeFrom, timeUntil, Operation::Type::ARC_ADDITION);
}

unsigned long long DynamicDiGraph::countArcRemovals(unsigned long long timeFrom, unsigned long long timeUntil) const
{
    return grin->countOperations(timeFrom, timeUntil, Operation::Type::ARC_REMOVAL);
}

void DynamicDiGraph::squashTimes(unsigned long long timeFrom, unsigned long long timeUntil)
{
    grin->reset();
    grin->squashTimes(timeFrom, timeUntil);
}

void DynamicDiGraph::secondArcIsRemoval(bool sir)
{
    grin->doubleArcIsRemoval = sir;
}

}
