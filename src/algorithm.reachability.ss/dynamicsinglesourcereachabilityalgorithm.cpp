#include "dynamicsinglesourcereachabilityalgorithm.h"

namespace Algora {

std::vector<Arc *> DynamicSingleSourceReachabilityAlgorithm::queryPath(const Vertex *)
{
    return std::vector<Arc*>();
}

bool DynamicSingleSourceReachabilityAlgorithm::prepare()
{
    return source != nullptr
            && DynamicDiGraphAlgorithm::prepare()
            && diGraph->containsVertex(source);
}


}
