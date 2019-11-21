#include "staticbfsapreachabilityalgorithm.h"

#include "algorithm.basic/finddipathalgorithm.h"

namespace Algora {

StaticBFSAPReachabilityAlgorithm::StaticBFSAPReachabilityAlgorithm(bool twoWayBFS)
    : DynamicAllPairsReachabilityAlgorithm(), twoWayBFS(twoWayBFS)
{
    registerEvents(false, false, false, false);
    fpa.setConstructPaths(false, false);
    fpa.useTwoWaySearch(twoWayBFS);
}

void StaticBFSAPReachabilityAlgorithm::run()
{  }

bool StaticBFSAPReachabilityAlgorithm::query(Vertex *s, Vertex *t)
{
    fpa.setConstructPaths(false, false);
    fpa.setSourceAndTarget(s, t);
    // fpa.prepare() omitted for performance reasons
    fpa.run();
    return fpa.deliver();
}

std::vector<Arc *> StaticBFSAPReachabilityAlgorithm::queryPath(Vertex *s, Vertex *t)
{
    fpa.setConstructPaths(false, true);
    fpa.setSourceAndTarget(s, t);
    // fpa.prepare() omitted for performance reasons
    fpa.run();
    if (fpa.deliver()) {
        return fpa.deliverArcsOnPath();
    }
    return std::vector<Arc*>();
}

void StaticBFSAPReachabilityAlgorithm::onDiGraphSet()
{
    bfsStepSize = static_cast<DiGraph::size_type>(
                ceil(diGraph->getNumArcs(true) / diGraph->getSize()));
    if (bfsStepSize < 5) {
        bfsStepSize = 5;
    }
    fpa.setGraph(diGraph);
    fpa.setTwoWayStepSize(bfsStepSize);
}

}
