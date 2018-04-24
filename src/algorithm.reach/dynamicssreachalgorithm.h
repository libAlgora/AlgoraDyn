#ifndef DYNAMICSSREACHALGORITHM_H
#define DYNAMICSSREACHALGORITHM_H

#include "algorithm/dynamicdigraphalgorithm.h"
#include "graph/digraph.h"

namespace Algora {

class DynamicSSReachAlgorithm : public DynamicDiGraphAlgorithm
{
public:
    explicit DynamicSSReachAlgorithm() : DynamicDiGraphAlgorithm(), source(0) { }
    virtual ~DynamicSSReachAlgorithm() { }

    void setSource(Vertex *s) { source = s; }
    virtual bool query(const Vertex *t) = 0;

    // DiGraphAlgorithm interface
public:
    virtual bool prepare() override { return source != 0 && DynamicDiGraphAlgorithm::prepare() && diGraph->containsVertex(source); }

protected:
    Vertex *source;
};

}

#endif // DYNAMICSSREACHALGORITHM_H
