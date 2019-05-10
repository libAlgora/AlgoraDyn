#ifndef RANDOMQUERYGENERATOR_H
#define RANDOMQUERYGENERATOR_H

#include <random>
#include <tuple>

namespace Algora {

class DynamicDiGraph;
class Vertex;

//template <unsigned int V = 1, unsigned int A = 0>
class RandomQueryGenerator
{
    enum struct NUM_QUERY_RELATION : std::int8_t { TIMEDIFF_IN_DELTA, OPS_IN_DELTA };
public:
    RandomQueryGenerator();

    void setSeed(unsigned long long s) { seed = s; initialized = false; }
    void setAbsoluteNumberOfQueries(unsigned int n) { absoluteQueries = n; relativeQueries = 0.0; }
    void setRelativeNumberOfQueries(double n, const NUM_QUERY_RELATION &r) { relativeQueries = n; relateTo = r; absoluteQueries = 0.0; }

    //std::vector<std::tuple<Vertex*>> generateQueries();
    std::vector<Vertex*> generateVertexQueries(const DynamicDiGraph *dyGraph);


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
