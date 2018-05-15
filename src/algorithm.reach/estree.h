#ifndef ESTREE_H
#define ESTREE_H

#include "dynamicssreachalgorithm.h"
#include "property/propertymap.h"

namespace Algora {

class ESTree : public DynamicSSReachAlgorithm
{
public:
    struct VertexData;
    explicit ESTree();
    virtual ~ESTree();

    // DiGraphAlgorithm interface
public:
    virtual void run() override;
    virtual std::string getName() const noexcept override { return "ES-Tree Single-Source Reachability Algorithm"; }
    virtual std::string getShortName() const noexcept override { return "EST-DSSReach"; }

protected:
    virtual void onDiGraphUnset() override;

    // DynamicDiGraphAlgorithm interface
protected:
    virtual void onArcRemove(Arc *a) override;

    // DynamicSSReachAlgorithm interface
public:
    virtual bool query(const Vertex *t) override;

private:
    PropertyMap<VertexData*> data;
    Vertex *root;
    bool initialized;
};

}

#endif // ESTREE_H
