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

#include "dynamicdigraph.h"
#include "dynamicdigraphoperations.h"

#include "graph.incidencelist/incidencelistgraph.h"
#include "graph.incidencelist/incidencelistvertex.h"

#include <vector>
#include "property/fastpropertymap.h"
#include "datastructure/circularbucketlist.h"

#include <stdexcept>
#include <cassert>
#include <limits>

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


struct DynamicDiGraph::CheshireCat {
    IncidenceListGraph dynGraph;
    IncidenceListGraph constructionGraph;

    std::vector<DynamicTime> timestamps;
    std::vector<Operation*> operations;
    std::vector<DynamicDiGraph::size_type> offset;
    OperationSet antedated;

    DynamicDiGraph::size_type timeIndex;
    DynamicDiGraph::size_type opIndex;

    bool doubleArcIsRemoval;
    bool removeIsolatedEnds;

    size_type defaultArcAge;
    CircularBucketList<AddArcOperation*> autoArcRemovals;

    std::vector<AddVertexOperation*> vertices;
    FastPropertyMap<AddArcOperation*> constructionArcMap;

    FastPropertyMap<VertexIdentifier> vertexToIdMap;
    DynamicDiGraph::size_type vertexToIdMapNextOpIndex;

    unsigned long numResets;
    DiGraph::size_type curVertexSize;
    DiGraph::size_type curArcSize;
    DiGraph::size_type maxVertexSize;
    DiGraph::size_type maxArcSize;

    DiGraph::size_type minVertexId;

    bool graphChangedSinceLastReset;


    CheshireCat() : timeIndex(0U), opIndex(0U), doubleArcIsRemoval(false),
        removeIsolatedEnds(false),
        defaultArcAge(1U),
        autoArcRemovals(defaultArcAge),
        vertexToIdMapNextOpIndex(0ULL), numResets(0U),
        curVertexSize(0ULL), curArcSize(0ULL), maxVertexSize(0ULL), maxArcSize(0ULL),
        minVertexId(std::numeric_limits<DiGraph::size_type>::infinity()),
        graphChangedSinceLastReset(false) {
        constructionArcMap.setDefaultValue(nullptr);
        vertexToIdMap.setDefaultValue(0U);
        clear();
    }
    ~CheshireCat() {
        clear();
    }

    void reset() {
        timeIndex = 0U;
        opIndex = 0U;
        if (graphChangedSinceLastReset) {
            dynGraph.clearAndRelease();
            dynGraph.reserveVertexCapacity(minVertexId + maxVertexSize);
            dynGraph.reserveArcCapacity(maxArcSize);
        } else {
            dynGraph.clearOrderedly();
        }
        //vertexToIdMap.clear();
        vertexToIdMap.resetAll(minVertexId + maxVertexSize);
        vertexToIdMapNextOpIndex = 0ULL;

        antedated.reset();
        for (auto op : operations) {
            op->reset();
        }
        numResets++;

        graphChangedSinceLastReset = false;
    }

    void init() {
        if (!antedated.operations.empty()) {
            antedated.apply(&dynGraph);
        }
    }

    void clear() {
        reset();
        vertices.clear();
        constructionArcMap.resetAll(0);
        constructionGraph.clear();
        timestamps.clear();
        curVertexSize = 0ULL;
        curArcSize = 0ULL;
        maxVertexSize = 0ULL;
        maxArcSize = 0ULL;
        minVertexId = std::numeric_limits<DiGraph::size_type>::infinity();

        antedated.clear();
        for (auto op : operations) {
            delete op;
        }
        operations.clear();
        offset.clear();
    }

    void checkTimestamp(DynamicTime timestamp) {
        if (!timestamps.empty() && timestamp < timestamps.back()) {
            throw std::invalid_argument("Timestamps must be non-decreasing.");
        }
        extendTime(timestamp);
    }

    void extendTime(DynamicTime timestamp) {
        if (timestamps.empty() || timestamps.back() < timestamp) {
            //PRINT_DEBUG( "Extending time from "
            //            << (timestamps.empty() ? 0U : timestamps.back()) << " to " << timestamp )
            timestamps.push_back(timestamp);
            offset.push_back(operations.size());

            // run before all other operations
            for (auto *aao : autoArcRemovals.front()) {
                removeArc(aao, this->removeIsolatedEnds);
            }
            autoArcRemovals.front().clear();
            autoArcRemovals.shift();
        }
    }

    AddVertexOperation *createAddVertexOperation(DynamicTime timestamp, bool atEnd, VertexIdentifier vertexId = 0U,
                               bool okIfExists = false) {
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
        vertices[vertexId] = avo;

        curVertexSize++;
        if (curVertexSize > maxVertexSize) {
            maxVertexSize = curVertexSize;
        }
        if (vertexId < minVertexId) {
            minVertexId = vertexId;
        }

        graphChangedSinceLastReset = true;

        return avo;
    }

    VertexIdentifier addVertex(DynamicTime timestamp, bool atEnd, VertexIdentifier vertexId = 0U,
                               bool okIfExists = false) {
        auto *avo = createAddVertexOperation(timestamp, atEnd, vertexId, okIfExists);
        operations.push_back(avo);

        return avo->vertexId;
    }

    void removeVertex(VertexIdentifier vertexId, DynamicTime timestamp) {
        if (vertexId >= vertices.size() || vertices.at(vertexId) == nullptr) {
            throw std::invalid_argument("Vertex ID does not exist.");
        }

        checkTimestamp(timestamp);

        AddVertexOperation *avo = vertices[vertexId];

        auto removeIncidentArcs = [&](Arc *a) {
            constructionArcMap.resetToDefault(a);
        };

        constructionGraph.mapOutgoingArcs(avo->constructionVertex, removeIncidentArcs);
        constructionGraph.mapIncomingArcs(avo->constructionVertex, removeIncidentArcs);

        constructionGraph.removeVertex(avo->constructionVertex);
        RemoveVertexOperation *rvo = new RemoveVertexOperation(avo);
        operations.push_back(rvo);
        vertices[vertexId] = nullptr;

        curVertexSize--;

        graphChangedSinceLastReset = true;
    }

    AddArcOperation *addArc(VertexIdentifier tailId, VertexIdentifier headId, DynamicTime timestamp, bool antedateVertexAddition)
    {
        checkTimestamp(timestamp);

        auto maxId = tailId > headId ? tailId : headId;
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

        graphChangedSinceLastReset = true;
        return aao;
    }

    void addArcAndRemoveIn(VertexIdentifier tailId, VertexIdentifier headId, DynamicTime timestamp,
                           size_type arcAge,
                           bool antedateVertexAddition)
    {
        if (arcAge < 1) {
            arcAge = defaultArcAge;
        }
        auto *aao = addArc(tailId, headId, timestamp, antedateVertexAddition);
        if (arcAge > autoArcRemovals.size()) {
            autoArcRemovals.resize(arcAge);
        }
        autoArcRemovals[arcAge - 1].push_back(aao);
    }

    void noop(DynamicTime timestamp) {
        checkTimestamp(timestamp);
        NoOperation *nop = new NoOperation;
        operations.push_back(nop);
    }

    Arc *findArc(VertexIdentifier tailId, VertexIdentifier headId) {
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
        if (constructionGraph.getOutDegree(ct, true) <= constructionGraph.getInDegree(ch, true)) {
            constructionGraph.mapOutgoingArcsUntil(ct, [&ch,&ca](Arc *a) {
                if (a->getHead() == ch) {
                    ca = a;
                }
            }, [&ca](const Arc*) { return ca != nullptr; });
        } else {
            constructionGraph.mapIncomingArcsUntil(ch, [&ct,&ca](Arc *a) {
                if (a->getTail() == ct) {
                    ca = a;
                }
            }, [&ca](const Arc*) { return ca != nullptr; });
        }

        return ca;
    }

    AddArcOperation *findAddArcOperation(VertexIdentifier tailId, VertexIdentifier headId) {
        Arc *ca = findArc(tailId, headId);
        if (!ca) {
            return nullptr;
        }

        AddArcOperation *aao = constructionArcMap[ca];
        assert(ca == aao->constructionArc);
        return aao;
    }


    void removeArc(VertexIdentifier tailId, VertexIdentifier headId, DynamicTime timestamp, bool removeIsolatedEnds) {
        AddArcOperation *aao = findAddArcOperation(tailId, headId);
        if (!aao) {
            throw std::invalid_argument("Arc does not exist.");
        }
        checkTimestamp(timestamp);
        removeArc(aao, removeIsolatedEnds);
    }

    void removeArc(AddArcOperation *aao, bool removeIsolatedEnds) {
        RemoveArcOperation *rao = new RemoveArcOperation(aao);
        auto *ca = aao->constructionArc;
        constructionGraph.removeArc(ca);
        constructionArcMap.resetToDefault(ca);

        if (removeIsolatedEnds) {
            AddVertexOperation *avoTail = aao->tail;
            AddVertexOperation *avoHead = aao->head;
            IncidenceListVertex *tail = dynamic_cast<IncidenceListVertex*>(avoTail->constructionVertex);
            IncidenceListVertex *head = dynamic_cast<IncidenceListVertex*>(avoHead->constructionVertex);
            if (tail->isIsolated() || head->isIsolated()) {
                OperationSet *op = new OperationSet;
                op->operations.push_back(rao);
                if (tail->isIsolated()) {
                    constructionGraph.removeVertex(avoTail->constructionVertex);
                    RemoveVertexOperation *rvo = new RemoveVertexOperation(avoTail);
                    op->operations.push_back(rvo);
                    vertices[avoTail->vertexId] = nullptr;
                    curVertexSize--;
                }
                if (avoTail != avoHead && head->isIsolated()) {
                    constructionGraph.removeVertex(avoHead->constructionVertex);
                    RemoveVertexOperation *rvo = new RemoveVertexOperation(avoHead);
                    op->operations.push_back(rvo);
                    vertices[avoHead->vertexId] = nullptr;
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

        graphChangedSinceLastReset = true;
    }

    void compact(DynamicDiGraph::size_type num) {
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

        auto maxOp = operations.size();
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

    auto findTimeIndex(DynamicTime timestamp, long long offset = 0) const {
        auto lower = std::lower_bound(timestamps.begin() + offset, timestamps.end(), timestamp);
        return std::distance(timestamps.cbegin(), lower);
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

    DynamicDiGraph::size_type countOperations(DynamicTime timeFrom, DynamicTime timeUntil,
                                              Operation::Type type) const {
        if (timeUntil < timeFrom || timeFrom > timestamps.back()) {
            return 0U;
        }
        auto tIndexFrom = findTimeIndex(timeFrom);
        assert(tIndexFrom >= 0);
        auto from = static_cast<DynamicDiGraph::size_type>(tIndexFrom);
        if (from >= timestamps.size()) {
                return 0U;
        }
        auto tIndexUntil = findTimeIndex(timeUntil, tIndexFrom);
        assert(tIndexUntil >= 0);
        auto until = static_cast<DynamicDiGraph::size_type>(tIndexUntil) + 1;
        auto opIndexMax = until < offset.size() ? offset[until] : operations.size();
        DynamicDiGraph::size_type numOperations = 0U;
        for (auto opI = offset[from]; opI < opIndexMax; opI++) {
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

    void squashTimes(DynamicTime timeFrom, DynamicTime timeUntil) {
        auto squashOn = findTimeIndex(timeFrom);
        auto squashMax = findTimeIndex(timeUntil) + 1;
        timestamps.erase(timestamps.cbegin() + squashOn + 1, timestamps.cbegin() + squashMax);
        offset.erase(offset.cbegin() + squashOn + 1, offset.cbegin() + squashMax);
    }

    Vertex *vertexForId(VertexIdentifier vertexId) const {
        if (vertexId >= vertices.size() || vertices.at(vertexId) == nullptr) {
            return nullptr;
        }
        return vertices.at(vertexId)->vertex;
    }

    DynamicDiGraph::size_type getSizeOfLastDelta() {
        if (timeIndex + 1U == offset.size()) {
            return operations.size() - offset[timeIndex];
        }
        return offset[timeIndex + 1U] - offset[timeIndex];
    }

    DynamicDiGraph::size_type getSizeOfFinalDelta() {
        return operations.size() - offset.back();
    }

    VertexIdentifier idOfIthVertex(DynamicDiGraph::size_type i) {

        PRINT_DEBUG("Id of " << i <<  "th vertex requested.");
        PRINT_DEBUG("Size of dynamic graph is " << dynGraph.getSize() << ".");
        std::function<void(Operation*)> updateMap;
        updateMap = [this,&updateMap](Operation *op) {
            if (op->getType() == Operation::Type::VERTEX_ADDITION) {
                AddVertexOperation *avo = dynamic_cast<AddVertexOperation*>(op);
                vertexToIdMap[avo->vertex] = avo->vertexId;
                PRINT_DEBUG("Mapped " << avo->vertex <<  " to id " << avo->vertexId << ".");
            } else if (op->getType() == Operation::Type::VERTEX_REMOVAL) {
                RemoveVertexOperation *rvo = dynamic_cast<RemoveVertexOperation*>(op);
                vertexToIdMap.resetToDefault(rvo->addOp->vertex);
                PRINT_DEBUG("Removed mapping of " << rvo->addOp->vertex << ".");
            } else if (op->getType() == Operation::Type::MULTIPLE) {
                OperationSet *os = dynamic_cast<OperationSet*>(op);
                for (auto o : os->operations) {
                    updateMap(o);
                }
            }
        };

        if (vertexToIdMapNextOpIndex == 0U) {
            updateMap(&antedated);
        }
        for (;vertexToIdMapNextOpIndex < opIndex; vertexToIdMapNextOpIndex++) {
            Operation *op = operations[vertexToIdMapNextOpIndex];
            updateMap(op);
        }
        return vertexToIdMap(dynGraph.vertexAt(i));
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

IncidenceListGraph *DynamicDiGraph::getDiGraph() const
{
    return &(grin->dynGraph);
}

DynamicDiGraph::DynamicTime DynamicDiGraph::getCurrentTime() const
{
    return grin->timestamps.empty() || (grin->timeIndex == 0U && grin->opIndex == 0U)
            ? 0U : grin->timestamps[grin->timeIndex];
}

DynamicDiGraph::DynamicTime DynamicDiGraph::getTimeOfXthNextDelta(DynamicTime x, bool forward) const
{
    if (grin->timestamps.empty()) {
        return 0U;
    }
    auto tIndex = grin->timeIndex;
    if (!forward && x > tIndex) {
        tIndex = 0ULL;
    } else if (!forward) {
        tIndex -= x;
    } else {
        tIndex += x;
        if (tIndex >= grin->timestamps.size()) {
            tIndex = grin->timestamps.size() - 1U;
        }
    }

    return grin->timestamps[tIndex];
}

DynamicDiGraph::DynamicTime DynamicDiGraph::getMaxTime() const
{
    return grin->timestamps.empty() ? 0U : grin->timestamps.back();
}

const std::vector<DynamicDiGraph::DynamicTime> &DynamicDiGraph::getTimestamps() const
{
    return grin->timestamps;
}

DynamicDiGraph::size_type DynamicDiGraph::getNumberOfDeltas() const
{
    return grin->timestamps.size();
}

DiGraph::size_type DynamicDiGraph::getConstructedGraphSize() const
{
    return grin->constructionGraph.getSize();
}

DiGraph::size_type DynamicDiGraph::getConstructedArcSize() const
{
    return grin->constructionGraph.getNumArcs(true);
}

GraphArtifact::size_type DynamicDiGraph::getMinVertexId() const
{
    return grin->minVertexId;
}

GraphArtifact::size_type DynamicDiGraph::getMaxVertexId() const
{
    return grin->minVertexId + grin->maxVertexSize - 1;
}

DynamicDiGraph::VertexIdentifier DynamicDiGraph::addVertex(DynamicTime timestamp)
{
    return grin->addVertex(timestamp, true);
}

void DynamicDiGraph::addVertex(VertexIdentifier vertexId, DynamicTime timestamp)
{
    grin->addVertex(timestamp, false, vertexId);
}

void DynamicDiGraph::removeVertex(VertexIdentifier vertexId, DynamicTime timestamp)
{
    grin->removeVertex(vertexId, timestamp);
}

void DynamicDiGraph::addArc(VertexIdentifier tailId, VertexIdentifier headId,
                            DynamicTime timestamp, bool antedateVertexAdditions)
{
    if (grin->doubleArcIsRemoval && hasArc(tailId, headId)) {
        grin->removeArc(tailId, headId, timestamp, antedateVertexAdditions);
    } else {
        grin->addArc(tailId, headId, timestamp, antedateVertexAdditions);
    }
}

void DynamicDiGraph::addArcAndRemoveIn(VertexIdentifier tailId, VertexIdentifier headId,
                                       DynamicTime timestamp, size_type ageInDeltas,
                                       bool antedateVertexAdditions)
{
    grin->addArcAndRemoveIn(tailId, headId, timestamp, ageInDeltas, antedateVertexAdditions);
}

void DynamicDiGraph::removeArc(VertexIdentifier tailId, VertexIdentifier headId,
                               DynamicTime timestamp)
{
    grin->removeArc(tailId, headId, timestamp, grin->removeIsolatedEnds);
}

void DynamicDiGraph::removeArc(VertexIdentifier tailId, VertexIdentifier headId,
                               DynamicTime timestamp, bool removeIsolatedEnds)
{
    grin->removeArc(tailId, headId, timestamp, removeIsolatedEnds);
}

void DynamicDiGraph::noop(DynamicDiGraph::DynamicTime timestamp)
{
    grin->noop(timestamp);
}

bool DynamicDiGraph::hasArc(VertexIdentifier tailId, VertexIdentifier headId)
{
    return grin->findArc(tailId, headId) != nullptr;
}

void DynamicDiGraph::clear()
{
    grin->clear();
}

void DynamicDiGraph::compact(DynamicDiGraph::size_type num)
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

bool DynamicDiGraph::lastOpWasNoop() const
{
    return grin->lastOpHadType(Operation::Type::NONE);
}

Vertex *DynamicDiGraph::getCurrentVertexForId(VertexIdentifier vertexId) const
{
    return grin->vertexForId(vertexId);
}

DynamicDiGraph::VertexIdentifier DynamicDiGraph::idOfIthVertex(DynamicDiGraph::size_type i)
{
    return grin->idOfIthVertex(i);
}

DynamicDiGraph::size_type DynamicDiGraph::getSizeOfLastDelta() const
{
    return grin->getSizeOfLastDelta();
}

DynamicDiGraph::size_type DynamicDiGraph::getSizeOfFinalDelta() const
{
    return grin->getSizeOfFinalDelta();
}

DynamicDiGraph::size_type DynamicDiGraph::countVertexAdditions(DynamicTime timeFrom,
                                                               DynamicTime timeUntil) const
{
    return grin->countOperations(timeFrom, timeUntil, Operation::Type::VERTEX_ADDITION);
}

DynamicDiGraph::size_type DynamicDiGraph::countVertexRemovals(DynamicTime timeFrom,
                                                              DynamicTime timeUntil) const
{
    return grin->countOperations(timeFrom, timeUntil, Operation::Type::VERTEX_REMOVAL);
}

DynamicDiGraph::size_type DynamicDiGraph::countArcAdditions(DynamicTime timeFrom,
                                                            DynamicTime timeUntil) const
{
    return grin->countOperations(timeFrom, timeUntil, Operation::Type::ARC_ADDITION);
}

DynamicDiGraph::size_type DynamicDiGraph::countArcRemovals(DynamicTime timeFrom,
                                                           DynamicTime timeUntil) const
{
    return grin->countOperations(timeFrom, timeUntil, Operation::Type::ARC_REMOVAL);
}

DynamicDiGraph::size_type DynamicDiGraph::countNoops(DynamicDiGraph::DynamicTime timeFrom,
                                                     DynamicDiGraph::DynamicTime timeUntil) const
{
    return grin->countOperations(timeFrom, timeUntil, Operation::Type::NONE);
}

void DynamicDiGraph::squashTimes(DynamicTime timeFrom, DynamicTime timeUntil)
{
    grin->reset();
    grin->squashTimes(timeFrom, timeUntil);
}

void DynamicDiGraph::secondArcIsRemoval(bool sir)
{
    grin->doubleArcIsRemoval = sir;
}

void DynamicDiGraph::setDefaultArcAge(DynamicDiGraph::size_type defaultAge)
{
    if (defaultAge > grin->defaultArcAge) {
        grin->autoArcRemovals.resize(defaultAge);
    }
    grin->defaultArcAge = defaultAge;
}

DynamicDiGraph::size_type DynamicDiGraph::getDefaultArcAge() const
{
    return grin->defaultArcAge;
}

void DynamicDiGraph::setRemoveIsolatedEnds(bool remove)
{
    grin->removeIsolatedEnds = remove;
}

bool DynamicDiGraph::removeIsolatedEnds() const
{
    return grin->removeIsolatedEnds;
}

void DynamicDiGraph::addOperation(DynamicTime timestamp, Operation *op)
{
    grin->checkTimestamp(timestamp);
    grin->operations.push_back(op);
}

Operation *DynamicDiGraph::getLastOperation() const
{
    assert(!grin->operations.empty());
    return grin->operations.back();
}

void DynamicDiGraph::replaceLastOperation(Operation *op)
{
    assert(!grin->operations.empty());
    grin->operations[grin->operations.size() - 1] = op;
}

AddArcOperation *DynamicDiGraph::findAddArcOperation(DynamicDiGraph::VertexIdentifier tailId, DynamicDiGraph::VertexIdentifier headId)
{
    return grin->findAddArcOperation(tailId, headId);
}

void DynamicDiGraph::removeArc(AddArcOperation *aao)
{
    grin->removeArc(aao, grin->removeIsolatedEnds);
}

}
