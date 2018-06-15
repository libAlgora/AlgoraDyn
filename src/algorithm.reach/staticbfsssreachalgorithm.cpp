#include "staticbfsssreachalgorithm.h"

#include "algorithm.basic/breadthfirstsearch.h"

namespace Algora {

StaticBFSSSReachAlgorithm::StaticBFSSSReachAlgorithm()
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
    static BreadthFirstSearch bfs(false);
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
