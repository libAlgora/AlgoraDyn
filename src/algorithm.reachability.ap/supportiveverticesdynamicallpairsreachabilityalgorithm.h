// license
#ifndef SUPPORTIVEVERTICESDYNAMICALLPAIRSREACHABILITYALGORITHM_H
#define SUPPORTIVEVERTICESDYNAMICALLPAIRSREACHABILITYALGORITHM_H

#include "dynamicallpairsreachabilityalgorithm.h"
#include "property/fastpropertymap.h"
#include "graph/digraph.h"

namespace Algora {

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm>
class SupportiveVerticesDynamicAllPairsReachabilityAlgorithm
        : public DynamicAllPairsReachabilityAlgorithm
{
public:
    typedef typename DynamicSingleSourceAlgorithm::ParameterSet SingleSourceParameterSet;
    typedef typename DynamicSingleSinkAlgorithm::ParameterSet SingleSinkParameterSet;

    explicit SupportiveVerticesDynamicAllPairsReachabilityAlgorithm(double supportSizeRatio,
                                                                    bool ssrSubtreeCheck);
    explicit SupportiveVerticesDynamicAllPairsReachabilityAlgorithm(double supportSizeRatio,
            bool ssrSubtreeCheck,
            const SingleSourceParameterSet &ssourceParams,
            const SingleSinkParameterSet &ssinkParams);
    virtual ~SupportiveVerticesDynamicAllPairsReachabilityAlgorithm();

    // DiGraphAlgorithm interface
public:
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

    // DiGraphAlgorithm interface
protected:
    virtual void onDiGraphSet() override;
    virtual void onDiGraphUnset() override;

    // DynamicAllPairsReachabilityAlgorithm interface
public:
    virtual bool query(Vertex *s, Vertex *t) override;
    virtual std::vector<Arc *> queryPath(Vertex *, Vertex *) override;

private:
    typedef std::pair<DynamicSingleSourceAlgorithm*,DynamicSingleSinkAlgorithm*> SSRPair;
    SingleSourceParameterSet ssourceParameters;
    SingleSinkParameterSet ssinkParameters;
    FastPropertyMap<SSRPair> supportiveVertexToSSRAlgorithm;
    std::vector<SSRPair> supportiveSSRAlgorithms;
    bool initialized;
    double supportSizeRatio;
    DiGraph::size_type twoWayStepSize;
    bool ssrSubtreeCheck;

    profiling_counter min_supportive_vertices = 0;
    profiling_counter max_supportive_vertices = 0;
    profiling_counter supportive_ssr_hits = 0;
//    profiling_counter ssr_subtree_checks = 0;
//    profiling_counter ssr_subtree_hits = 0;
//    profiling_counter forward_bfs_total_steps = 0;
//    profiling_counter backward_bfs_total_steps = 0;
//    profiling_counter num_query_resume = 0;
    profiling_counter num_trivial_queries = 0;
    profiling_counter num_only_ssr_queries = 0;
    profiling_counter num_only_support_queries = 0;

    void reset();
    void createAndInitAlgorithm(Vertex *v);
};

}

#include "supportiveverticesdynamicallpairsreachabilityalgorithm.cpp"

#endif // SUPPORTIVEVERTICESDYNAMICALLPAIRSREACHABILITYALGORITHM_H
