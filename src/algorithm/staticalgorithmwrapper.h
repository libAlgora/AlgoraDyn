#ifndef STATICALGORITHMWRAPPER_H
#define STATICALGORITHMWRAPPER_H

#include "dynamicdigraphalgorithm.h"

namespace Algora {

class StaticAlgorithmWrapper
        : public DynamicDiGraphAlgorithm
{
public:
    explicit StaticAlgorithmWrapper(DiGraphAlgorithm *a, bool rva = true, bool rvr = true,
                                    bool raa = true, bool rar = true);
    virtual ~StaticAlgorithmWrapper() { delete staticAlgorithm; }

    // DiGraphAlgorithm interface
public:
    virtual bool prepare() override { return staticAlgorithm->prepare(); }
    virtual void run() override { staticAlgorithm->run(); }
    virtual std::string getName() const noexcept override { return "Dynamized " + staticAlgorithm->getName(); }
    virtual std::string getShortName() const noexcept override { return "dyz-" + staticAlgorithm->getShortName(); }

protected:
    virtual void onDiGraphSet() override {
        DynamicDiGraphAlgorithm::onDiGraphSet();
        staticAlgorithm->setGraph(diGraph);
    }

    // DynamicDiGraphAlgorithm interface
protected:
    virtual void onVertexAdd(Vertex *) override;
    virtual void onVertexRemove(Vertex *) override;
    virtual void onArcAdd(Arc *) override;
    virtual void onArcRemove(Arc *) override;

private:
    DiGraphAlgorithm *staticAlgorithm;
    bool recomputeOnVertexAdded;
    bool recomputeOnVertexRemoved;
    bool recomputeOnArcAdded;
    bool recomputeOnArcRemoved;

    void rerunIf(bool rerun);
};

}

#endif // STATICALGORITHMWRAPPER_H
