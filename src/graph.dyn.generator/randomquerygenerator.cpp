#include "randomquerygenerator.h"

#include <functional>

namespace Algora {

RandomQueryGenerator::RandomQueryGenerator()
    : absoluteQueries(0ULL), relativeQueries(0.0),
      relateTo(NUM_QUERY_RELATION::OPS_IN_DELTA), seed(0ULL), initialized(false)
{

}

RandomQueryGenerator::VertexQueryList RandomQueryGenerator::generateVertexQueries(
        const DynamicDiGraph *dyGraph)
{
    init();

    auto numQueries = computeNumQueries(dyGraph);
    VertexQueryList queries;
    // assuming that vertex ids are consecutive
    std::uniform_int_distribution<DynamicDiGraph::VertexIdentifier> distVertex(
                0, dyGraph->getDiGraph()->getSize() - 1);
    auto randomVertex = std::bind(distVertex, std::ref(gen));

    for (auto i = numQueries; i > 0; i--) {
        queries.push_back(randomVertex());
    }

    return queries;
}

std::vector<RandomQueryGenerator::VertexQueryList> RandomQueryGenerator::provideVertexQueries(
        DynamicDiGraph *dyGraph)
{
    init();
    std::vector<VertexQueryList> queriesSet;

    dyGraph->resetToBigBang();
    auto sumQueries = 0Ull;

    while (dyGraph->applyNextDelta()) {
        std::uniform_int_distribution<DynamicDiGraph::VertexIdentifier> distVertex(
                    0, dyGraph->getDiGraph()->getSize() - 1);
        auto randomVertex = std::bind(distVertex, std::ref(gen));

        queriesSet.emplace_back();
        auto &queries = queriesSet.back();
        auto numQueries = computeNumQueries(dyGraph);
        sumQueries += numQueries;
        for (auto i = 0Ull; i < numQueries; i++) {
            queries.push_back(dyGraph->idOfIthVertex(randomVertex()));
        }
    }

    // Special treatment for NOOPs at end
    if (dyGraph->lastOpWasNoop() && queriesSet.back().empty()) {
        queriesSet.pop_back();
    }

    return queriesSet;

}

void RandomQueryGenerator::init()
{
    if (initialized) {
        return;
    }

    if (seed == 0ULL) {
        std::random_device rd;
        seed = rd();
    }

    gen.seed(seed);
    initialized = true;
}

DynamicDiGraph::size_type RandomQueryGenerator::computeNumQueries(const DynamicDiGraph *dyGraph) const
{
    auto numQueries = absoluteQueries;
    if (relativeQueries > 0.0) {
        auto tsCur = dyGraph->getCurrentTime();
        auto tsNext = dyGraph->getTimeOfXthNextDelta(1, true);
        switch (relateTo) {
        case NUM_QUERY_RELATION::TIMEDIFF_IN_DELTA:
            numQueries = tsNext - tsCur;
            break;
        case NUM_QUERY_RELATION::OPS_IN_DELTA:
            numQueries = dyGraph->countArcAdditions(tsCur, tsCur) + dyGraph->countArcRemovals(tsCur, tsCur);
            break;
        }
        numQueries = static_cast<DynamicDiGraph::size_type>(round(numQueries * relativeQueries));
    }
    return numQueries;
}

}
