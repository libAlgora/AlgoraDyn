#ifndef DYNAMICALLPAIRSREACHABILITYALGORITHM_H
#define DYNAMICALLPAIRSREACHABILITYALGORITHM_H

#include "algorithm/dynamicdigraphalgorithm.h"

namespace Algora {

class DynamicAllPairsReachabilityAlgorithm : public DynamicDiGraphAlgorithm
{
public:
    explicit DynamicAllPairsReachabilityAlgorithm() = default;
    virtual ~DynamicAllPairsReachabilityAlgorithm() override = default;

    virtual bool query(Vertex *s, Vertex *t) = 0;
    virtual std::vector<Arc*> queryPath(Vertex *, Vertex *);
};

}

#endif // DYNAMICALLPAIRSREACHABILITYALGORITHM_H
