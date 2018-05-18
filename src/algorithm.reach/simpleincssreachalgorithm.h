#ifndef SIMPLEINCSSREACHALGORITHM_H
#define SIMPLEINCSSREACHALGORITHM_H

#include "dynamicssreachalgorithm.h"

#include "property/propertymap.h"

namespace Algora {

class SimpleIncSSReachAlgorithm : public DynamicSSReachAlgorithm
{
public:
    explicit SimpleIncSSReachAlgorithm();
    virtual ~SimpleIncSSReachAlgorithm();

    // DiGraphAlgorithm interface
public:
    virtual void run() override;
    virtual std::string getName() const noexcept override { return "Simple Incremental Single-Source Reachability Algorithm"; }
    virtual std::string getShortName() const noexcept override { return "Simple-ISSReach"; }

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
    virtual void dumpData(std::ostream &os) override;

private:
    struct Reachability;
    Reachability *data;
    bool initialized;
};

}

#endif // SIMPLEINCSSREACHALGORITHM_H
