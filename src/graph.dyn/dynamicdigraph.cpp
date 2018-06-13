#include "dynamicdigraph.h"

#include "graph.incidencelist/incidencelistgraph.h"

#include <vector>
#include <unordered_map>

namespace Algora {

struct Operation {
    virtual ~Operation() {}
    virtual void apply(IncidenceListGraph *graph) = 0;
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
};

struct AddVertexOperation : public Operation {
    Vertex *vertex;
    Vertex *constructionVertex;

    AddVertexOperation(Vertex *cv) : vertex(nullptr), constructionVertex(cv) { }

    virtual void apply(IncidenceListGraph *graph) {
        vertex = graph->addVertex();
    }
};

struct RemoveVertexOperation : public Operation {
    AddVertexOperation *addOp;

    RemoveVertexOperation(AddVertexOperation *avo) : addOp(avo) {}

    virtual void apply(IncidenceListGraph *graph) {
        graph->removeVertex(addOp->vertex);
    }
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
};

struct RemoveArcOperation : public Operation {
    AddArcOperation *addOp;

    RemoveArcOperation(AddArcOperation *aao) : addOp(aao) { }

    virtual void apply(IncidenceListGraph *graph) {
        graph->removeArc(addOp->arc);
    }
};

struct DynamicDiGraph::CheshireCat {
    IncidenceListGraph dynGraph;
    IncidenceListGraph constructionGraph;

    std::vector<unsigned int> timestamps;
    std::vector<std::vector<Operation*>> operations;

    unsigned int timeIndex;
    unsigned int opIndex;

    std::vector<AddVertexOperation*> vertices;
    std::unordered_map<Arc*,AddArcOperation*> constructionArcMap;

    CheshireCat() : timeIndex(0U), opIndex(0U) { }
    ~CheshireCat() {
        for (auto opList : operations) {
            for (auto op: opList) {
               delete op;
            }
        }
    }

    void reset() {
        timeIndex = 0U;
        opIndex = 0U;
        dynGraph.clear();
    }

    void clear() {
        reset();
        vertices.clear();
        constructionArcMap.clear();
        constructionGraph.clear();
        timestamps.clear();

        for (auto opList : operations) {
            for (auto op: opList) {
               delete op;
            }
        }
        operations.clear();
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
            operations.emplace_back();
        }
    }

    void addVertex(unsigned int timestamp) {
        checkTimestamp(timestamp);

        Vertex *cv = constructionGraph.addVertex();
        AddVertexOperation *avo = new AddVertexOperation(cv);
        operations.back().push_back(avo);
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
        operations.back().push_back(rvo);
        vertices.erase(vertices.cbegin() + vertexId);
    }

    void addArc(unsigned int tailId, unsigned int headId, unsigned int timestamp)
    {
        checkTimestamp(timestamp);

        unsigned int maxId = tailId > headId ? tailId : headId;

        OperationSet *os = nullptr;
        if (maxId >= vertices.size()) {
            os = new OperationSet;
        }
        while (vertices.size() <= maxId) {
           //addVertex(timestamp);
            Vertex *cv = constructionGraph.addVertex();
            AddVertexOperation *avo = new AddVertexOperation(cv);
            os->operations.push_back(avo);
            vertices.push_back(avo);
        }

        Arc *ca = constructionGraph.addArc(vertices[tailId]->constructionVertex, vertices[headId]->constructionVertex);
        AddArcOperation *aao = new AddArcOperation(vertices[tailId], vertices[headId], ca);
        if (os) {
            os->operations.push_back(aao);
            operations.back().push_back(os);
        } else {
            operations.back().push_back(aao);
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
        //if (tailId >= vertices.size() || headId >= vertices.size()) {
        //    throw std::invalid_argument("Tail or head ID does not exist.");
        //}

        //Vertex *ct = vertices[tailId]->constructionVertex;
        //Vertex *ch = vertices[headId]->constructionVertex;
        //Arc *ca = nullptr;
        //constructionGraph.mapOutgoingArcsUntil(ct, [&](Arc *a) {
        //    if (a->getHead() == ch) {
        //        ca = a;
        //    }
        //}, [&](const Arc*) { return ca != nullptr; });

        Arc *ca = findArc(tailId, headId);
        if (!ca) {
            throw std::invalid_argument("Arc does not exist.");
        }

        checkTimestamp(timestamp);

        AddArcOperation *aao = constructionArcMap[ca];
        RemoveArcOperation *rao = new RemoveArcOperation(aao);
        constructionGraph.removeArc(aao->constructionArc);
        constructionArcMap.erase(aao->constructionArc);
        operations.back().push_back(rao);
    }

    bool advance() {
        if (timestamps.empty()
                || (timeIndex + 1 == timestamps.size() && opIndex == operations[timeIndex].size())) {
            return false;
        }

        if (opIndex == operations[timeIndex].size()) {
            opIndex = 0;
            timeIndex++;
        }
        return true;
    }

    bool nextOp() {
        if (!advance()) {
            return false;
        }
        operations[timeIndex][opIndex]->apply(&dynGraph);
        opIndex++;
        return true;
    }

    bool nextDelta() {
        if (!advance()) {
            return false;
        }

        while (opIndex < operations[timeIndex].size()) {
            operations[timeIndex][opIndex]->apply(&dynGraph);
            opIndex++;
        }
        return true;
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

void DynamicDiGraph::addVertex(unsigned int timestamp)
{
    grin->addVertex(timestamp);
}

void DynamicDiGraph::removeVertex(unsigned int vertexId, unsigned int timestamp)
{
    grin->removeVertex(vertexId, timestamp);
}

void DynamicDiGraph::addArc(unsigned int tailId, unsigned int headId, unsigned int timestamp)
{
    grin->addArc(tailId, headId, timestamp);
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


}