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
    initialized = true;
}

bool CachingBFSSSReachAlgorithm::query(const Vertex *t)
{
    if (t == source) {
        return true;
    }
    if (!initialized) {
        if (!prepare()) {
            throw DiGraphAlgorithmException(this, "Could not prepare myself.");
        }
        run();
    }
    return levels(t) != -1;
}

void CachingBFSSSReachAlgorithm::onDiGraphSet()
{
    DynamicSSReachAlgorithm::onDiGraphSet();
    bfs.setGraph(diGraph);
    initialized = false;
}

void CachingBFSSSReachAlgorithm::onDiGraphUnset()
{
    bfs.setGraph(diGraph);
    bfs.unsetGraph();
    initialized = false;
    DynamicSSReachAlgorithm::onDiGraphUnset();
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
