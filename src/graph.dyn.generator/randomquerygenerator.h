#ifndef RANDOMQUERYGENERATOR_H
#define RANDOMQUERYGENERATOR_H

#include <random>
#include <tuple>

#include "graph.dyn/dynamicdigraph.h"
#include "pipe/dynamicdigraphqueryprovider.h"

namespace Algora {

class DynamicDiGraph;
class Vertex;

class RandomQueryGenerator : public DynamicDiGraphQueryProvider
{
public:
    enum struct NUM_QUERY_RELATION : std::int8_t { TIMEDIFF_IN_DELTA, OPS_IN_DELTA };

    RandomQueryGenerator();

    void setSeed(unsigned long long s) { seed = s; initialized = false; }
    void setAbsoluteNumberOfQueries(unsigned int n) { absoluteQueries = n; relativeQueries = 0.0; }
    void setRelativeNumberOfQueries(double n, const NUM_QUERY_RELATION &r) { relativeQueries = n; relateTo = r; absoluteQueries = 0.0; }
    unsigned long long getSeed() const { return seed; }

    VertexQueryList generateVertexQueries(const DynamicDiGraph *dyGraph);
    std::vector<VertexQueryList> provideVertexQueries(DynamicDiGraph *dyGraph) override;

    // DynamicDiGraphQueryProvider interface
public:
    virtual std::string getName() const noexcept override { return "Random Query Generator"; }

private:
    unsigned long long absoluteQueries;
    double relativeQueries;
    NUM_QUERY_RELATION relateTo;

    std::mt19937_64 gen;
    unsigned long long seed;
    bool initialized;

    void init();
    unsigned long long computeNumQueries(const DynamicDiGraph *dyGraph) const;
};

}

#endif // RANDOMQUERYGENERATOR_H