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
    virtual void onVertexAdd(Vertex *v) override;
    virtual void onArcAdd(Arc *a) override;
    virtual void onVertexRemove(Vertex *v) override;
    virtual void onArcRemove(Arc *a) override;

    // DynamicSSReachAlgorithm interface
protected:
    virtual void onSourceSet() override;

    // DynamicSSReachAlgorithm interface
public:
    virtual bool query(const Vertex *t) override;
    virtual void dumpData(std::ostream &os) override;

private:
    PropertyMap<VertexData*> data;
    Vertex *root;
    bool initialized;

    void restoreTree(VertexData *rd);
    void cleanup();

};

}

#endif // ESTREE_H
