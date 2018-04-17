#include "dynamicdigraphalgorithm.h"

#include "graph/digraph.h"
#include <functional>

namespace Algora {

void DynamicDiGraphAlgorithm::onDiGraphSet()
{
    //auto f3 = std::bind(&Foo::print_sum, &foo, 95, _1);
    //    f3(5);
    using namespace std::placeholders;  // for _1, _2, _3...
    //auto ova = std::bind(&DynamicDiGraphAlgorithm::onVertexAdd, this, _1);
    diGraph->onVertexAdd(this, std::bind(&DynamicDiGraphAlgorithm::onVertexAdd, this, _1));
    diGraph->onVertexRemove(this, std::bind(&DynamicDiGraphAlgorithm::onVertexRemove, this, _1));
    diGraph->onArcAdd(this, std::bind(&DynamicDiGraphAlgorithm::onArcAdd, this, _1));
    diGraph->onArcRemove(this, std::bind(&DynamicDiGraphAlgorithm::onArcRemove, this, _1));
}

void DynamicDiGraphAlgorithm::onDiGraphUnset()
{
    if (!diGraph)
        return;
    diGraph->removeOnVertexAdd(this);
    diGraph->removeOnVertexRemove(this);
    diGraph->removeOnArcAdd(this);
    diGraph->removeOnArcRemove(this);
}

}
