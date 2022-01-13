#ifndef DYNAMICWEIGHTEDDIGRAPHALGORITHM_H
#define DYNAMICWEIGHTEDDIGRAPHALGORITHM_H

#include "dynamicdigraphalgorithm.h"
#include "property/modifiableproperty.h"

namespace Algora {

template<typename weight_type>
class DynamicWeightedDiGraphAlgorithm
        : public DynamicDiGraphAlgorithm
{
public:
    explicit DynamicWeightedDiGraphAlgorithm()
        : DynamicDiGraphAlgorithm(),
          weights(nullptr),
          registerPropertyChange(true),
          registered(false) { }

    virtual ~DynamicWeightedDiGraphAlgorithm() override {
        deregisterAsPropertyObserver();
    }

    void setWeights(ModifiableProperty<weight_type> *w) {
        if (weights != w) {
            onWeightsUnset();
            weights = w;
            onWeightsSet();
            registerAsPropertyObserver();
        }
    }

    void unsetWeights() {
        deregisterAsPropertyObserver();
        onWeightsUnset();
        weights = nullptr;
    }

    virtual void onPropertyChange(GraphArtifact *,
                                  const weight_type &/*oldValue*/,
                                  const weight_type &/*newValue*/) { }

    // DynamicDiGraphAlgorithm interface
    virtual void setAutoUpdate(bool au) override {
        DynamicDiGraphAlgorithm::setAutoUpdate(au);

        if (!au && registered) {
            deregisterAsPropertyObserver();
        } else if (au && !registered) {
            registerAsPropertyObserver();
        }
    }

    // DiGraphAlgorithm interface
    virtual bool prepare() override {
        return     DynamicDiGraphAlgorithm::prepare()
                && weights != nullptr;
    }

protected:
    ModifiableProperty<weight_type> *weights;

    void registerPropertyEvents(bool propertyChange) {
        registerPropertyChange = propertyChange;
    }

    virtual void onWeightsSet() { }
    virtual void onWeightsUnset() { }

private:
    bool registerPropertyChange;
    bool registered;

    void registerAsPropertyObserver() {
        if (registerPropertyChange && weights && doesAutoUpdate()) {
            using namespace std::placeholders;  // for _1, _2, _3...
            weights->onPropertyChange(this,
                                 std::bind(&DynamicWeightedDiGraphAlgorithm::onPropertyChange,
                                           this,
                                           _1, _2, _3));
            registered = true;
        }

    }

    void deregisterAsPropertyObserver() {
        if (weights && registered) {
            weights->removeOnPropertyChange(this);
            registered = false;
        }
    }
};

}

#endif // DYNAMICWEIGHTEDDIGRAPHALGORITHM_H
