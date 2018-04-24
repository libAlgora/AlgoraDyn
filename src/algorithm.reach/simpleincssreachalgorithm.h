#ifndef SIMPLEINCSSREACHALGORITHM_H
#define SIMPLEINCSSREACHALGORITHM_H

#include "dynamicssreachalgorithm.h"

#include "property/propertymap.h"

namespace Algora {

class SimpleIncSSReachAlgorithm : public DynamicSSReachAlgorithm
{
public:
    SimpleIncSSReachAlgorithm();

    // DiGraphAlgorithm interface
public:
    virtual void run() override;
    virtual std::string getName() const noexcept override { return "Simple Incremental Single-Source Reachability Algorithm"; }
    virtual std::string getShortName() const noexcept override { return "SimpleIncSSReach"; }

protected:
    virtual void onDiGraphUnset() override;

    // DynamicDiGraphAlgorithm interface
protected:
    virtual void onVertexAdd(Vertex *) override;
    virtual void onVertexRemove(Vertex *v) override;
    virtual void onArcAdd(Arc *a) override;
    virtual void onArcRemove(Arc *a) override;

    // DynamicSSReachAlgorithm interface
public:
    virtual bool query(const Vertex *t) override;

private:
    PropertyMap<int> isReachable;
    bool initialized;

    void reachFrom(const Vertex *s);
};

}

#endif // SIMPLEINCSSREACHALGORITHM_H
