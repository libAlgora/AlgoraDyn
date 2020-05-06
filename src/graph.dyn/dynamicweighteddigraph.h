#ifndef DYNAMICWEIGHTEDDIGRAPH_H
#define DYNAMICWEIGHTEDDIGRAPH_H

#include "dynamicdigraph.h"
#include "property/fastpropertymap.h"
#include "dynamicdigraphoperations.h"

namespace Algora {

template<typename weight_type,
         template<typename T>
         typename propertymap_type = FastPropertyMap>
class DynamicWeightedDiGraph : public DynamicDiGraph
{
public:
    explicit DynamicWeightedDiGraph(weight_type defaultWeight = weight_type())
        : defaultWeight(defaultWeight), weights(defaultWeight)
    { }
    virtual ~DynamicWeightedDiGraph() = default;

    void addWeightedArc(VertexIdentifier tailId,
                VertexIdentifier headId,
                const weight_type &weight,
                DynamicTime timestamp,
                bool antedateVertexAdditions = false) {
        DynamicDiGraph::addArc(tailId, headId, timestamp, antedateVertexAdditions);
        auto *lastOp = getLastOperation();
        if (lastOp->getType() == Operation::MULTIPLE) {
            OperationSet *os = dynamic_cast<OperationSet*>(lastOp);
            auto *aao = dynamic_cast<AddArcOperation*>(os->operations.back());
            assert(aao);
            auto *wco = new ArcWeightChangeOperation<weight_type>(&weights, aao, weight);
            os->operations.push_back(wco);
        } else {
            auto *aao = dynamic_cast<AddArcOperation*>(lastOp);
            assert(aao);
            auto *wco = new ArcWeightChangeOperation<weight_type>(&weights, aao, weight);

            auto *os = new OperationSet;
            os->operations.push_back(aao);
            os->operations.push_back(wco);
            replaceLastOperation(os);
        }
    }

    void changeArcWeight(VertexIdentifier tailId,
                VertexIdentifier headId,
                const weight_type &weight,
                DynamicTime timestamp) {
        auto *aao = findAddArcOperation(tailId, headId);
        auto *wco = new ArcWeightChangeOperation<weight_type>(&weights, aao, weight);
        addOperation(timestamp, wco);
    }

    propertymap_type<weight_type> *getArcWeights() {
        return &weights;
    }

private:
    weight_type defaultWeight;
    propertymap_type<weight_type> weights;
};

}

#endif // DYNAMICWEIGHTEDDIGRAPH_H
