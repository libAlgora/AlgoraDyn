#include "cachingbfsssreachalgorithm.h"
#include "algorithm/digraphalgorithmexception.h"

namespace Algora {

CachingBFSSSReachAlgorithm::CachingBFSSSReachAlgorithm()
    : DynamicSSReachAlgorithm(), initialized(false)
{
    levels.setDefaultValue(-1);
    bfs.usePropertyMap(&levels);
    bfs.orderAsValues(false);
}

CachingBFSSSReachAlgorithm::~CachingBFSSSReachAlgorithm()
{

}

void CachingBFSSSReachAlgorithm::run()
{
    levels.resetAll();
    bfs.setStartVertex(source);
    if (!bfs.prepare()) {
       throw DiGraphAlgorithmException(this, "Could not prepare BFS algorithm.");
    }
    bfs.run();
    bfs.deliver();
}

bool CachingBFSSSReachAlgorithm::query(const Vertex *t)
{
    if (!initialized) {
        prepare();
        run();
    }
    return levels(t) != -1;
}

void CachingBFSSSReachAlgorithm::onDiGraphSet()
{
    bfs.setGraph(diGraph);
}

void CachingBFSSSReachAlgorithm::onDiGraphUnset()
{
    bfs.unsetGraph();
}

void CachingBFSSSReachAlgorithm::onArcAdd(Arc *)
{
    initialized = false;
}

void CachingBFSSSReachAlgorithm::onArcRemove(Arc *)
{
    initialized = false;
}

void CachingBFSSSReachAlgorithm::onSourceSet()
{
    initialized = false;
}

} // end namespace
