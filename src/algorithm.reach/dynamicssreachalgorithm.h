#ifndef DYNAMICSSREACHALGORITHM_H
#define DYNAMICSSREACHALGORITHM_H

#include "algorithm/dynamicdigraphalgorithm.h"
#include "graph/digraph.h"
#include <ostream>

namespace Algora {

class DynamicSSReachAlgorithm : public DynamicDiGraphAlgorithm
{
public:
    explicit DynamicSSReachAlgorithm() : DynamicDiGraphAlgorithm(), source(nullptr) { }
    virtual ~DynamicSSReachAlgorithm() { }

    void setSource(Vertex *s) { source = s; onSourceSet(); }
    virtual bool query(const Vertex *t) = 0;

    virtual void dumpData(std::ostream&) { }

    // DiGraphAlgorithm interface
public:
    virtual bool prepare() override { return source != nullptr && DynamicDiGraphAlgorithm::prepare() && diGraph->containsVertex(source); }

protected:
    virtual void onSourceSet() { }

    Vertex *source;
};

}

#endif // DYNAMICSSREACHALGORITHM_H
