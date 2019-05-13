#ifndef RANDOMQUERYGENERATOR_H
#define RANDOMQUERYGENERATOR_H

#include <random>
#include <tuple>

#include "graph.dyn/dynamicdigraph.h"

namespace Algora {

class DynamicDiGraph;
class Vertex;

class RandomQueryGenerator
{
public:
    typedef std::vector<DynamicDiGraph::VertexIdentifier> VertexQueryList;
    enum struct NUM_QUERY_RELATION : std::int8_t { TIMEDIFF_IN_DELTA, OPS_IN_DELTA };

    RandomQueryGenerator();

    void setSeed(unsigned long long s) { seed = s; initialized = false; }
    void setAbsoluteNumberOfQueries(unsigned int n) { absoluteQueries = n; relativeQueries = 0.0; }
    void setRelativeNumberOfQueries(double n, const NUM_QUERY_RELATION &r) { relativeQueries = n; relateTo = r; absoluteQueries = 0.0; }

    //std::vector<std::tuple<Vertex*>> generateQueries();
    VertexQueryList generateVertexQueries(const DynamicDiGraph *dyGraph);
    std::vector<VertexQueryList> generateAllVertexQueries(DynamicDiGraph *dyGraph);


private:
    unsigned long long absoluteQueries;
    double relativeQueries;
    NUM_QUERY_RELATION relateTo;

    unsigned long long seed;
    std::mt19937_64 gen;
    bool initialized;

    void init();
    unsigned long long computeNumQueries(const DynamicDiGraph *dyGraph) const;
};

}

#endif // RANDOMQUERYGENERATOR_H
