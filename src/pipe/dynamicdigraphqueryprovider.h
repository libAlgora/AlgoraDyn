#ifndef DYNAMICDIGRAPHQUERYPROVIDER_H
#define DYNAMICDIGRAPHQUERYPROVIDER_H

#include "graph.dyn/dynamicdigraph.h"
#include <vector>
#include <string>

namespace Algora {

class DynamicDiGraphQueryProvider
{
public:
    typedef std::vector<DynamicDiGraph::VertexIdentifier> VertexQueryList;
    DynamicDiGraphQueryProvider();
    virtual ~DynamicDiGraphQueryProvider();

    virtual std::vector<VertexQueryList> provideVertexQueries(DynamicDiGraph *dyGraph) = 0;
    virtual std::string getName() const noexcept { return "Dynamic Digraph Query Provider"; }
};

}

#endif // DYNAMICDIGRAPHQUERYPROVIDER_H
