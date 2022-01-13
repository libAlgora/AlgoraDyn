#ifndef DYNAMICWEIGHTEDDIGRAPH_H
#define DYNAMICWEIGHTEDDIGRAPH_H

#include "dynamicdigraph.h"
#include "property/fastpropertymap.h"
#include "dynamicdigraphoperations.h"

namespace Algora {

template<typename W,
         template<typename T>
         typename propertymap_type = FastPropertyMap>
class DynamicWeightedDiGraph : public DynamicDiGraph
{
public:
    typedef W weight_type;

    explicit DynamicWeightedDiGraph(weight_type defaultWeight = weight_type())
        : defaultWeight(defaultWeight), weights(defaultWeight), constructionWeights(defaultWeight)
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
            constructionWeights[aao->constructionArc] = weight;
            os->operations.push_back(wco);
        } else {
            auto *aao = dynamic_cast<AddArcOperation*>(lastOp);
            assert(aao);
            auto *wco = new ArcWeightChangeOperation<weight_type>(&weights, aao, weight);
            constructionWeights[aao->constructionArc] = weight;

            auto *os = new OperationSet;
            os->operations.push_back(aao);
            os->operations.push_back(wco);
            replaceLastOperation(os);
        }
    }

    void addWeightedArcOrChangeWeight(VertexIdentifier tailId,
                VertexIdentifier headId,
                const weight_type &weight,
                DynamicTime timestamp,
                bool antedateVertexAdditions = false) {
        auto *aao = findAddArcOperation(tailId, headId);
        if (aao) {
            changeArcWeight(aao, weight, timestamp);
        } else {
            addWeightedArc(tailId, headId, weight, timestamp, antedateVertexAdditions);
        }
    }

    void addWeightedArcOrChangeWeightRelative(VertexIdentifier tailId,
                VertexIdentifier headId,
                const weight_type &weight,
                bool increase,
                bool removeIfNonPositive,
                DynamicTime timestamp,
                bool antedateVertexAdditions = false) {
        auto *aao = findAddArcOperation(tailId, headId);
        if (aao) {
            changeArcWeightRelative(aao, weight, increase, removeIfNonPositive, timestamp);
        } else {
            addWeightedArc(tailId, headId, weight, timestamp, antedateVertexAdditions);
        }
    }

    void changeArcWeight(VertexIdentifier tailId,
                VertexIdentifier headId,
                const weight_type &weight,
                DynamicTime timestamp) {
        auto *aao = findAddArcOperation(tailId, headId);
        if (aao) {
            changeArcWeight(aao, weight, timestamp);
        }
    }

    void changeArcWeightRelative(VertexIdentifier tailId,
                VertexIdentifier headId,
                const weight_type &weight,
                bool increase,
                bool removeIfNonPositive,
                DynamicTime timestamp) {
        auto *aao = findAddArcOperation(tailId, headId);
        if (aao) {
            changeArcWeightRelative(aao, weight, increase, removeIfNonPositive, timestamp);
        }
    }

    void removeWeightedArc(VertexIdentifier tailId,
                VertexIdentifier headId,
                DynamicTime timestamp) {
        auto *aao = findAddArcOperation(tailId, headId);
        if (!aao) {
            throw std::invalid_argument("Arc does not exist.");
        }
        removeWeightedArc(aao, timestamp);
    }

    weight_type getCurrentArcWeight(VertexIdentifier tailId, VertexIdentifier headId) {
        auto *aao = findAddArcOperation(tailId, headId);
        if (aao) {
            return constructionWeights[aao->constructionArc];
        }
        return constructionWeights.getDefaultValue();
    }

    propertymap_type<weight_type> *getArcWeights() {
        return &weights;
    }

private:
    weight_type defaultWeight;
    propertymap_type<weight_type> weights;
    propertymap_type<weight_type> constructionWeights;

    void changeArcWeight(AddArcOperation *aao, const weight_type &weight, DynamicTime timestamp) {
        addOperation(timestamp,
                new ArcWeightChangeOperation<weight_type>(&weights, aao, weight));
        constructionWeights[aao->constructionArc] = weight;
    }

    void changeArcWeightRelative(AddArcOperation *aao, const weight_type &weight, bool increase,
                                 bool removeIfNonPositive, DynamicTime timestamp) {
        auto newWeight = constructionWeights(aao->constructionArc);
        if (increase) {
            newWeight += weight;
        } else {
            if (removeIfNonPositive && weight > newWeight) {
                removeWeightedArc(aao, timestamp);
                return;
            } else {
                newWeight -= weight;
            }
        }
        constructionWeights[aao->constructionArc] = newWeight;
        addOperation(timestamp,
                new ArcWeightChangeOperation<weight_type>(&weights, aao, newWeight));
    }

    void removeWeightedArc(AddArcOperation *aao, DynamicTime timestamp) {
        checkTimestamp(timestamp);
        constructionWeights.resetToDefault(aao->constructionArc);
        removeArc(aao);
    }
};

}

#endif // DYNAMICWEIGHTEDDIGRAPH_H
