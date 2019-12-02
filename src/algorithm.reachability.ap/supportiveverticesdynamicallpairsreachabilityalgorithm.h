// license
#ifndef SUPPORTIVEVERTICESDYNAMICALLPAIRSREACHABILITYALGORITHM_H
#define SUPPORTIVEVERTICESDYNAMICALLPAIRSREACHABILITYALGORITHM_H

#include "dynamicallpairsreachabilityalgorithm.h"
#include "property/fastpropertymap.h"
#include "graph/digraph.h"

namespace Algora {

template<typename DynamicSSRAlgorithm>
class SupportiveVerticesDynamicAllPairsReachabilityAlgorithm
        : public DynamicAllPairsReachabilityAlgorithm
{
public:
    explicit SupportiveVerticesDynamicAllPairsReachabilityAlgorithm(double supportSizeRatio);
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
    FastPropertyMap<DynamicSSRAlgorithm*> supportiveVertexToSSRAlgorithm;
    std::vector<DynamicSSRAlgorithm*> supportiveSSRAlgorithms;
    bool initialized;
    double supportSizeRatio;
    DiGraph::size_type twoWayStepSize;

    profiling_counter min_supportive_vertices = 0;
    profiling_counter max_supportive_vertices = 0;
    profiling_counter supportive_ssr_hits = 0;
    profiling_counter known_unreachable_hits = 0;
    profiling_counter ssr_subtree_checks = 0;
    profiling_counter forward_bfs_total_steps = 0;
    profiling_counter backward_bfs_total_steps = 0;
    profiling_counter num_trivial_queries = 0;
    profiling_counter num_only_ssr_queries = 0;
    profiling_counter num_query_resume = 0;

    void reset();
    void createAndInitAlgorithm(Vertex *v);
};

}

#include "supportiveverticesdynamicallpairsreachabilityalgorithm.cpp"

#endif // SUPPORTIVEVERTICESDYNAMICALLPAIRSREACHABILITYALGORITHM_H
