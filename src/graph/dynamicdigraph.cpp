#include "dynamicdigraph.h"

#include "graph.incidencelist/incidencelistgraph.h"

#include <vector>
#include <unordered_map>

namespace Algora {

struct Operation {
    virtual void apply(IncidenceListGraph *graph) = 0;
};

struct AddVertexOperation : public Operation {
    Vertex *vertex;
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
    unsigned int tailId;
    unsigned int headId;
    Arc *arc;

    AddArcOperation(int tail, int head) : tailId(tail), headId(head) {}

    virtual void apply(IncidenceListGraph *graph) {
        arc = graph->addArc(graph->vertexAt(tailId), graph->vertexAt(headId));
    }
};

struct RemoveArcOperation : public Operation {
    AddArcOperation *addOp;
    RemoveArcOperation(AddArcOperation *aao) : addOp(aao){ }

    virtual void apply(IncidenceListGraph *graph) {
        graph->removeArc(addOp->arc);
    }
};

struct DynamicDiGraph::CheshireCat {
    IncidenceListGraph dynGraph;
    IncidenceListGraph constructionGraph;

    unsigned int dynTime;
    unsigned int constructionTime;

    std::vector<unsigned int> timestamps;
    std::vector<std::vector<Operation*>> operations;

    std::vector<AddVertexOperation*> vertices;
    std::unordered_map<std::pair<unsigned int,unsigned int>, std::vector<AddArcOperation*>> arcMap;

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
    return grin->dynTime;
}

unsigned int DynamicDiGraph::getMaxTime() const
{
    return grin->constructionTime;
}

void DynamicDiGraph::addVertex(unsigned int timestamp)
{
    grin->checkTimestamp(timestamp);

    AddVertexOperation *avo = new AddVertexOperation;
    grin->operations.back().push_back(avo);
    grin->vertices.push_back(avo);
}

void DynamicDiGraph::removeVertex(unsigned int vertexId, unsigned int timestamp)
{
    if (vertexId >= grin->vertices.size()) {
        throw std::invalid_argument("Vertex ID does not exist.");
    }

    grin->checkTimestamp(timestamp);

    AddVertexOperation *avo = grin->vertices[vertexId];
    RemoveVertexOperation *rvo = new RemoveVertexOperation(avo);
    grin->operations.back().push_back(rvo);
    grin->vertices.erase(grin->vertices.cbegin() + vertexId);

    // todo remove arcs...
}

void DynamicDiGraph::addArc(unsigned int tailId, unsigned int headId, unsigned int timestamp)
{
    grin->checkTimestamp(timestamp);

    unsigned int maxId = tailId > headId ? tailId : headId;
    for (int i = grin->vertices.size() - 1; i <= maxId; i++) {
        addVertex(timestamp);
    }

    AddArcOperation *aao = new AddArcOperation(tailId, headId);
    grin->operations.push_back(aao);
    grin->arcMap.insert(std::make_pair(tailId, headId), aao);
}

void DynamicDiGraph::removeArc(unsigned int tailId, unsigned int headId, unsigned int timestamp)
{

}

void DynamicDiGraph::resetToBigBang()
{
    grin->dynGraph.clear();
    grin->dynTime = 0U;
}

void DynamicDiGraph::applyNextOperation()
{

}

void DynamicDiGraph::applyNextDelta()
{

}


}
