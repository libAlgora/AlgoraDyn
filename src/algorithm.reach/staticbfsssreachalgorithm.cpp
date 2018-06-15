#include "staticbfsssreachalgorithm.h"

#include "algorithm.basic/breadthfirstsearch.h"

namespace Algora {

StaticBFSSSReachAlgorithm::StaticBFSSSReachAlgorithm()
    : DynamicSSReachAlgorithm()
{

}

StaticBFSSSReachAlgorithm::~StaticBFSSSReachAlgorithm()
{

}

void StaticBFSSSReachAlgorithm::run()
{

}

bool StaticBFSSSReachAlgorithm::query(const Vertex *t)
{
    if (t == source) {
        return true;
    }
    BreadthFirstSearch bfs(false);
    bfs.setStartVertex(source);
    bool reachable = false;
    bfs.setArcStopCondition([&](const Arc *a) {
        if (a->getHead() == t) {
            reachable = true;
        }
        return reachable;
    });
    runAlgorithm(bfs, diGraph);
    return reachable;
}

}
