#ifndef DYNAMICDIGRAPHOPERATIONS_H
#define DYNAMICDIGRAPHOPERATIONS_H

#include "graph.incidencelist/incidencelistgraph.h"
//#include "graph.dyn/dynamicdigraph.h"

namespace Algora {

struct Operation {
    enum Type {
      VERTEX_ADDITION, VERTEX_REMOVAL,
      ARC_ADDITION, ARC_REMOVAL,
      EDGE_ADDITION, EDGE_REMOVAL,
      MULTIPLE, NONE,
      VERTEX_WEIGHT_CHANGE, ARC_WEIGHT_CHANGE
    };
    virtual ~Operation() {}
    virtual void apply(IncidenceListGraph *graph) = 0;
    virtual Type getType() const = 0;
    virtual void reset() { }
};

struct OperationSet : public Operation {
    std::vector<Operation*> operations;
    Type type = MULTIPLE;

    void clear() {
        for (auto op : operations) {
            delete op;
        }
        operations.clear();
    }

    virtual ~OperationSet() override {
        clear();
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

struct NoOperation : public Operation {
    virtual void apply(IncidenceListGraph *) override {}
    virtual Type getType() const override { return NONE; }
};


struct AddVertexOperation : public Operation {
    Vertex *vertex;
    Vertex *constructionVertex;
    DynamicDiGraph::VertexIdentifier vertexId;

    AddVertexOperation(Vertex *cv, DynamicDiGraph::VertexIdentifier vId) : vertex(nullptr), constructionVertex(cv), vertexId(vId) { }

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
    const bool directed;

    AddArcOperation(AddVertexOperation *t, AddVertexOperation *h, Arc *ca)
        : tail(t), head(h), arc(nullptr), constructionArc(ca), directed(ca->isDirected()) { }

    virtual void apply(IncidenceListGraph *graph) override {
        arc = directed ?
          graph->addArc(tail->vertex, head->vertex)
          : graph->addEdge(tail->vertex, head->vertex);
    }
    virtual Type getType() const override { return directed ? ARC_ADDITION : EDGE_ADDITION; }
    virtual void reset() override { arc = nullptr; }
};

struct RemoveArcOperation : public Operation {
    AddArcOperation *addOp;

    RemoveArcOperation(AddArcOperation *aao) : addOp(aao) { }

    virtual void apply(IncidenceListGraph *graph) override {
      if (addOp->directed) {
        graph->removeArc(addOp->arc);
      } else {
        graph->removeEdge(addOp->arc);
      }
    }
    virtual Type getType() const override { return addOp->directed ? ARC_REMOVAL : EDGE_REMOVAL; }
};

template<typename weight_type>
struct VertexWeightChangeOperation : public Operation {

    ModifiableProperty<weight_type> *weights;
    AddVertexOperation *addVertex;
    weight_type weight;

    VertexWeightChangeOperation(ModifiableProperty<weight_type> *pm,
                          AddVertexOperation *avo,
                          const weight_type &newWeight)
        : weights(pm),  addVertex(avo), weight(newWeight) { }

    virtual void apply(IncidenceListGraph *) override {
        weights->setValue(addVertex->vertex, weight);
    }

    virtual Type getType() const override {
        return VERTEX_WEIGHT_CHANGE;
    }
};

template<typename weight_type>
struct ArcWeightChangeOperation : public Operation {

    ModifiableProperty<weight_type> *weights;
    AddArcOperation *addArc;
    weight_type weight;

    ArcWeightChangeOperation(ModifiableProperty<weight_type> *pm,
                          AddArcOperation *aao,
                          const weight_type &newWeight)
        : weights(pm),  addArc(aao), weight(newWeight) { }

    virtual void apply(IncidenceListGraph *) override {
        weights->setValue(addArc->arc, weight);
    }

    virtual Type getType() const override {
        return ARC_WEIGHT_CHANGE;
    }
};

}

#endif // DYNAMICDIGRAPHOPERATIONS_H
