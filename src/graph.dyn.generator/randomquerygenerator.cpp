#include "randomquerygenerator.h"

#include "graph.dyn/dynamicdigraph.h"
//#include "graph/digraph.h"

#include <functional>

namespace Algora {

RandomQueryGenerator::RandomQueryGenerator()
    : absoluteQueries(0ULL), relativeQueries(0.0),
      relateTo(NUM_QUERY_RELATION::OPS_IN_DELTA), seed(0ULL), initialized(false)
{

}

std::vector<Vertex *> RandomQueryGenerator::generateVertexQueries(const DynamicDiGraph *dyGraph)
{
    init();

    auto numQueries = computeNumQueries(dyGraph);
    std::vector<Vertex*> queries;
    // assuming that vertex ids are consecutive
    std::uniform_int_distribution<unsigned long long> distVertex(0, dyGraph->getCurrentGraphSize() - 1);
    auto randomVertex = std::bind(distVertex, std::ref(gen));

    for (auto i = 0Ull; i < numQueries; i++) {
        queries.push_back(dyGraph->getCurrentVertexForId(randomVertex()));
    }

    return queries;
}

void RandomQueryGenerator::init()
{
    if (initialized) {
        return;
    }

    if (seed == 0U) {
        std::random_device rd;
        seed = rd();
    }

    gen.seed(seed);
    initialized = true;
}

unsigned long long RandomQueryGenerator::computeNumQueries(const DynamicDiGraph *dyGraph) const
{
    auto numQueries = absoluteQueries;
    if (relativeQueries > 0.0) {
        auto tsCur = dyGraph->getCurrentTime();
        auto tsPrev = dyGraph->getTimeOfXthNextDelta(1, false);
        if (tsPrev == tsCur) {
            return 0ULL;
        }
        switch (relateTo) {
        case NUM_QUERY_RELATION::TIMEDIFF_IN_DELTA:
            numQueries = tsCur - tsPrev;
            break;
        case NUM_QUERY_RELATION::OPS_IN_DELTA:
            numQueries = dyGraph->countArcAdditions(tsCur, tsCur) + dyGraph->countArcRemovals(tsCur, tsCur);
            break;
        }
    }
    return numQueries;
}

}
