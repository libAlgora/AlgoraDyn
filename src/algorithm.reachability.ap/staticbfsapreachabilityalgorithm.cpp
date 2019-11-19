#include "staticbfsapreachabilityalgorithm.h"

#include "algorithm.basic/finddipathalgorithm.h"

namespace Algora {

StaticBFSAPReachabilityAlgorithm::StaticBFSAPReachabilityAlgorithm(bool twoWayBFS)
    : DynamicAllPairsReachabilityAlgorithm(), twoWayBFS(twoWayBFS)
{  }

void StaticBFSAPReachabilityAlgorithm::run()
{  }

bool StaticBFSAPReachabilityAlgorithm::query(const Vertex *s, const Vertex *t)
{
    FindDiPathAlgorithm<FastPropertyMap> fpa(false, false, twoWayBFS);
    fpa.configure(diGraph, const_cast<Vertex*>(s), const_cast<Vertex*>(t));
    // omit fpa.prepare() for efficiency reasons
    fpa.run();
    return fpa.deliver();
}

std::vector<Arc *> StaticBFSAPReachabilityAlgorithm::queryPath(const Vertex *s, const Vertex *t)
{
    FindDiPathAlgorithm<FastPropertyMap> fpa(false, true, twoWayBFS);
    fpa.configure(diGraph, const_cast<Vertex*>(s), const_cast<Vertex*>(t));
    // omit checks fpa.prepare() for reasons of efficiency
    fpa.run();
    if (fpa.deliver()) {
        return fpa.deliverArcsOnPath();
    }
    return std::vector<Arc*>();
}


}
