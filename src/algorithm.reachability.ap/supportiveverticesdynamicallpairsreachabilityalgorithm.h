// license
#ifndef SUPPORTIVEVERTICESDYNAMICALLPAIRSREACHABILITYALGORITHM_H
#define SUPPORTIVEVERTICESDYNAMICALLPAIRSREACHABILITYALGORITHM_H

#include "dynamicallpairsreachabilityalgorithm.h"
#include "property/fastpropertymap.h"
#include "graph/digraph.h"

#include <random>

namespace Algora {

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm,
         bool reAdjust = false>
class SupportiveVerticesDynamicAllPairsReachabilityAlgorithm
        : public DynamicAllPairsReachabilityAlgorithm
{
public:
    // requeueLimit, maxAffectedRatio
    typedef std::tuple<double, unsigned long> ParameterSet;

    typedef typename DynamicSingleSourceAlgorithm::ParameterSet SingleSourceParameterSet;
    typedef typename DynamicSingleSinkAlgorithm::ParameterSet SingleSinkParameterSet;

    explicit SupportiveVerticesDynamicAllPairsReachabilityAlgorithm(double supportSize,
                                                                    unsigned long adjustAfter);
    explicit SupportiveVerticesDynamicAllPairsReachabilityAlgorithm(double supportSize,
            unsigned long adjustAfter,
            const SingleSourceParameterSet &ssourceParams,
            const SingleSinkParameterSet &ssinkParams);

    explicit SupportiveVerticesDynamicAllPairsReachabilityAlgorithm(
            const ParameterSet &params,
            const SingleSourceParameterSet &ssourceParams,
            const SingleSinkParameterSet &ssinkParams);
    virtual ~SupportiveVerticesDynamicAllPairsReachabilityAlgorithm();

    void setSeed(unsigned long long seed);

    // DiGraphAlgorithm interface
public:
    virtual bool prepare() override;
    virtual void run() override;
    virtual std::string getName() const noexcept override;
    virtual std::string getShortName() const noexcept override;
    virtual std::string getProfilingInfo() const override;

    // DynamicDiGraphAlgorithm interface
public:
    virtual void onVertexAdd(Vertex *v) override;
    virtual void onVertexRemove(Vertex *v) override;
    virtual void onArcAdd(Arc *a) override;
    virtual void onArcRemove(Arc *a) override;
    virtual Profile getProfile() const override;

    // DynamicAllPairsReachabilityAlgorithm interface
public:
    virtual bool query(Vertex *s, Vertex *t) override;
    virtual std::vector<Arc *> queryPath(Vertex *, Vertex *) override;

    // DiGraphAlgorithm interface
protected:
    virtual void onDiGraphSet() override;
    virtual void onDiGraphUnset() override;

    // own
protected:
    double supportSize;
    unsigned long adjustAfter;
    unsigned long adjustmentCountUp;
    SingleSourceParameterSet ssourceParameters;
    SingleSinkParameterSet ssinkParameters;
    unsigned long long seed;

    typedef std::pair<DynamicSingleSourceAlgorithm*,DynamicSingleSinkAlgorithm*> SSRPair;
    FastPropertyMap<SSRPair> supportiveVertexToSSRAlgorithm;
    std::vector<SSRPair> supportiveSSRAlgorithms;
    DiGraph::size_type twoWayStepSize;
    bool initialized;
    std::mt19937_64 gen;

    profiling_counter min_supportive_vertices = 0;
    profiling_counter max_supportive_vertices = 0;
    profiling_counter num_trivial_queries = 0;
    profiling_counter num_only_ssr_queries = 0;
    profiling_counter num_only_support_queries_svt = 0;
    profiling_counter num_only_support_queries_vs = 0;
    profiling_counter num_only_support_queries_tv = 0;
    profiling_counter num_expensive_queries = 0;
    profiling_counter num_adjustments = 0;

    void createAndInitAlgorithm(Vertex *v);
    void reset();

private:
    void pickSupportVertices(bool adjust);
};

}

#include "supportiveverticesdynamicallpairsreachabilityalgorithm.cpp"

#endif // SUPPORTIVEVERTICESDYNAMICALLPAIRSREACHABILITYALGORITHM_H
