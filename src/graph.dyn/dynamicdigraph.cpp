#include "dynamicdigraph.h"

#include "graph.incidencelist/incidencelistgraph.h"

#include <vector>
#include <unordered_map>

namespace Algora {

struct Operation {
    enum Type { VERTEX_ADDITION, VERTEX_REMOVAL, ARC_ADDITION, ARC_REMOVAL, MULTIPLE };
    virtual ~Operation() {}
    virtual void apply(IncidenceListGraph *graph) = 0;
    virtual Type getType() const = 0;
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
};

struct AddVertexOperation : public Operation {
    Vertex *vertex;
    Vertex *constructionVertex;

    AddVertexOperation(Vertex *cv) : vertex(nullptr), constructionVertex(cv) { }

    virtual void apply(IncidenceListGraph *graph) {
        vertex = graph->addVertex();
    }
    virtual Type getType() const override { return VERTEX_ADDITION; }
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

    std::vector<AddVertexOperation*> vertices;
    std::unordered_map<Arc*,AddArcOperation*> constructionArcMap;

    CheshireCat() : timeIndex(0U), opIndex(0U) { clear(); }
    ~CheshireCat() {
        clear();
    }

    void reset() {
        timeIndex = 0U;
        opIndex = 0U;
        dynGraph.clear();

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

        // do not delete 'antedated'...
        for (unsigned int i = 1U; i < operations.size(); i++) {
            delete operations[i];
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
            timestamps.push_back(timestamp);
            offset.push_back(operations.size());
        }
    }

    void addVertex(unsigned int timestamp) {
        checkTimestamp(timestamp);

        Vertex *cv = constructionGraph.addVertex();
        AddVertexOperation *avo = new AddVertexOperation(cv);
        operations.push_back(avo);
        vertices.push_back(avo);
    }

    void removeVertex(unsigned int vertexId, unsigned int timestamp) {
        if (vertexId >= vertices.size()) {
            throw std::invalid_argument("Vertex ID does not exist.");
        }

        checkTimestamp(timestamp);

        AddVertexOperation *avo = vertices[vertexId];

        auto removeIncidentArcs = [&](Arc *a) {
            constructionArcMap.erase(a);
        };

        constructionGraph.mapOutgoingArcs(avo->constructionVertex, removeIncidentArcs);
        constructionGraph.mapIncomingArcs(avo->constructionVertex, removeIncidentArcs);

        RemoveVertexOperation *rvo = new RemoveVertexOperation(avo);
        operations.push_back(rvo);
        vertices.erase(vertices.cbegin() + vertexId);
    }

    void addArc(unsigned int tailId, unsigned int headId, unsigned int timestamp, bool antedateVertexAddition)
    {
        checkTimestamp(timestamp);

        unsigned int maxId = tailId > headId ? tailId : headId;

        OperationSet *os = nullptr;
        if (maxId >= vertices.size()) {
            if (antedateVertexAddition && timeIndex == 0U) {
                os = &antedated;
            } else {
                os = new OperationSet;
            }
        }
        while (vertices.size() <= maxId) {
            Vertex *cv = constructionGraph.addVertex();
            AddVertexOperation *avo = new AddVertexOperation(cv);
            os->operations.push_back(avo);
            vertices.push_back(avo);
        }

        Arc *ca = constructionGraph.addArc(vertices[tailId]->constructionVertex, vertices[headId]->constructionVertex);
        AddArcOperation *aao = new AddArcOperation(vertices[tailId], vertices[headId], ca);
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
        Vertex *ct = vertices[tailId]->constructionVertex;
        Vertex *ch = vertices[headId]->constructionVertex;
        Arc *ca = nullptr;
        constructionGraph.mapOutgoingArcsUntil(ct, [&](Arc *a) {
            if (a->getHead() == ch) {
                ca = a;
            }
        }, [&](const Arc*) { return ca != nullptr; });

        return ca;
    }

    void removeArc(unsigned int tailId, unsigned int headId, unsigned int timestamp) {
        Arc *ca = findArc(tailId, headId);
        if (!ca) {
            throw std::invalid_argument("Arc does not exist.");
        }

        checkTimestamp(timestamp);

        AddArcOperation *aao = constructionArcMap[ca];
        RemoveArcOperation *rao = new RemoveArcOperation(aao);
        constructionGraph.removeArc(aao->constructionArc);
        constructionArcMap.erase(aao->constructionArc);
        operations.push_back(rao);
    }

    bool advance() {
        if (opIndex >= operations.size()) {
            return false;
        }

        if (opIndex == 0) {
            init();
        }

        if (timeIndex + 1 < timestamps.size() && opIndex == offset[timeIndex + 1]) {
            timeIndex++;
        }
        return true;
    }

    bool nextOp() {
        if (!advance()) {
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

    unsigned int countOperations(unsigned int timeFrom, unsigned int timeUntil, Operation::Type type) const {
        unsigned int tIndexUntil = findTimeIndex(timeUntil) + 1;
        unsigned int opIndexMax = tIndexUntil < offset.size() ? offset[tIndexUntil] : operations.size();
        unsigned int numOperations = 0U;
        for (unsigned int opIndex = offset[findTimeIndex(timeFrom)]; opIndex < opIndexMax; opIndex++) {
            Operation *op = operations[opIndex];
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
    return grin->vertices.size();
}

unsigned int DynamicDiGraph::getCurrentArcSize() const
{
   return grin->constructionArcMap.size();
}

void DynamicDiGraph::addVertex(unsigned int timestamp)
{
    grin->addVertex(timestamp);
}

void DynamicDiGraph::removeVertex(unsigned int vertexId, unsigned int timestamp)
{
    grin->removeVertex(vertexId, timestamp);
}

void DynamicDiGraph::addArc(unsigned int tailId, unsigned int headId, unsigned int timestamp, bool antedateVertexAdditions)
{
    grin->addArc(tailId, headId, timestamp, antedateVertexAdditions);
}

void DynamicDiGraph::removeArc(unsigned int tailId, unsigned int headId, unsigned int timestamp)
{
    grin->removeArc(tailId, headId, timestamp);
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

bool DynamicDiGraph::applyNextOperation()
{
    return grin->nextOp();
}

bool DynamicDiGraph::applyNextDelta()
{
    return grin->nextDelta();
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
    grin->squashTimes(timeFrom, timeUntil);
}

}
