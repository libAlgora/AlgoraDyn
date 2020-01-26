// license

#include "supportiveverticesdynamicallpairsreachabilityalgorithm.h"
#include "graph/digraph.h"
#include "graph.incidencelist/incidencelistgraph.h"
#include "graph.incidencelist/incidencelistvertex.h"
#include "algorithm.basic.traversal/breadthfirstsearch.h"
#include "algorithm.basic/finddipathalgorithm.h"

#include <sstream>

//#define DEBUG_SUPPVAPR

#ifdef DEBUG_SUPPVAPR
#include <iostream>
#undef PRINT_DEBUG
#define PRINT_DEBUG(msg) std::cerr << this->getShortName() << ": " << msg << std::endl;
#undef IF_DEBUG
#define IF_DEBUG(cmd) cmd;
#else
#ifndef PRINT_DEBUG
#define PRINT_DEBUG(msg) ((void)0)
#endif
#ifndef IF_DEBUG
#define IF_DEBUG(cmd)
#endif
#endif

namespace Algora {

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<
                                DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>
    ::SupportiveVerticesDynamicAllPairsReachabilityAlgorithm(
            double supportSize,
            unsigned long adjustAfter,
            const SingleSourceParameterSet &ssourceParams,
            const SingleSinkParameterSet &ssinkParams)
    : SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<
        DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>(
          std::make_pair(supportSize, adjustAfter), ssourceParams, ssinkParams)
{
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<
                                DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>
    ::SupportiveVerticesDynamicAllPairsReachabilityAlgorithm(
        const ParameterSet &params,
        const SingleSourceParameterSet &ssourceParams,
        const SingleSinkParameterSet &ssinkParams)
    : DynamicAllPairsReachabilityAlgorithm(),
      supportSize(std::get<0>(params)), adjustAfter(std::get<1>(params)), adjustmentCountUp(0),
      ssourceParameters(ssourceParams), ssinkParameters(ssinkParams),
      seed(0ULL), twoWayStepSize(5U), initialized(false)
{
    supportiveVertexToSSRAlgorithm.setDefaultValue({nullptr, nullptr});
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<
    DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>
    ::SupportiveVerticesDynamicAllPairsReachabilityAlgorithm(double supportSize,
                                                             unsigned long adjustAfter)
    : SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<
        DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>(
          std::make_pair(supportSize, adjustAfter), {}, {})
{ }

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<
    DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>
    ::~SupportiveVerticesDynamicAllPairsReachabilityAlgorithm()
{
    reset();
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
void
SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<
    DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>
::setSeed(unsigned long long seed)
{
    this->seed = seed;
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
bool SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<
    DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>::prepare()
{
    if (seed == 0ULL) {
        std::random_device rd;
        seed = rd();
    }
    // std::cout << "Seed is " << seed << "." << std::endl;
    gen.seed(seed);

    return true;
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
void SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<
    DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>::run()
{
    if (initialized) {
        return;
    }

    PRINT_DEBUG("Initializing...");
    pickSupportVertices(true);
    initialized = true;
    PRINT_DEBUG("Initialization complete.");

    if (reAdjust) {
        adjustmentCountUp = 0;
    }
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
std::string SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>::getName()
const noexcept
{
    DynamicSingleSourceAlgorithm ssrc(ssourceParameters);
    DynamicSingleSinkAlgorithm ssink(ssinkParameters);
    std::stringstream ss;
    ss << "Single-Source/Sink-Supported All-Pairs Reachability Algorithm ("
       << supportSize << ", ";
    if (reAdjust) {
        ss << "adjust-after=" << adjustAfter << ", ";
    } else {
        ss << "no-adjust, ";
    }
    ss << ssrc.getName() << ", "
       << ssink.getName() << ")";
    return ss.str();
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
std::string SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>
    ::getShortName() const noexcept
{
    DynamicSingleSourceAlgorithm ssrc(ssourceParameters);
    DynamicSingleSinkAlgorithm ssink(ssinkParameters);
    std::stringstream ss;
    ss << "SSR-Based APR("
       << supportSize << ", ";
    if (reAdjust) {
        ss << adjustAfter << "-adj, ";
    } else {
        ss << "no-adj, ";
    }
    ss << ssrc.getShortName() << ", "
       << ssink.getShortName() << ")";
    return ss.str();
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
std::string
SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>::getProfilingInfo()
const
{
    std::stringstream ss;
    ss << "Seed:                         " << seed << std::endl;
#ifdef COLLECT_PR_DATA
    ss << "#vertices considered:         " << pr_consideredVertices << std::endl;
    ss << "#arcs considered:             " << pr_consideredArcs << std::endl;
    ss << "#supportive vertices (min):   " << min_supportive_vertices << std::endl;
    ss << "#supportive vertices (max):   " << max_supportive_vertices << std::endl;
    ss << "#trivial queries:             " << num_trivial_queries << std::endl;
    ss << "#SSR-only queries:            " << num_only_ssr_queries << std::endl;
    ss << "#Support-only queries (svt):  " << num_only_support_queries_svt << std::endl;
    ss << "#Support-only queries (vs):   " << num_only_support_queries_vs << std::endl;
    ss << "#Support-only queries (tv):   " << num_only_support_queries_tv << std::endl;
    ss << "#Expensive queries:           " << num_expensive_queries << std::endl;
    ss << "#Adjustments:                 " << num_adjustments << std::endl;
#endif
    return ss.str();
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
void SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<DynamicSingleSourceAlgorithm,
                                                            DynamicSingleSinkAlgorithm, reAdjust>
    ::onVertexAdd(Vertex *v)
{
    PRINT_DEBUG("A vertex has been added: " << v);
    DynamicDiGraphAlgorithm::onVertexAdd(v);

    if (!initialized || supportSize == 0) {
        return;
    }

    bool readjusted = false;
    if (reAdjust) {
        adjustmentCountUp++;
        if (adjustmentCountUp >= adjustAfter) {
            pickSupportVertices(true);
            adjustmentCountUp = 0;
            readjusted = true;
        }
    }

    if (!doesAutoUpdate() && !readjusted) {
        for (auto &[ssrc, ssink] : supportiveSSRAlgorithms) {
            ssrc->onVertexAdd(v);
            ssink->onVertexAdd(v);
        }
    }

    if (!reAdjust) {
        pickSupportVertices(false);
    }

    if (max_supportive_vertices < supportiveSSRAlgorithms.size()) {
        max_supportive_vertices = supportiveSSRAlgorithms.size();
    }
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
void SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<
                                DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>
    ::onVertexRemove(Vertex *v)
{
    PRINT_DEBUG("A vertex is about to be deleted: " << v);
    DynamicDiGraphAlgorithm::onVertexRemove(v);

    if (!initialized || supportSize == 0) {
        return;
    }

    bool readjusted = false;
    if (reAdjust) {
        adjustmentCountUp++;
        if (adjustmentCountUp >= adjustAfter) {
            pickSupportVertices(true);
            adjustmentCountUp = 0;
            readjusted = true;
        }
    }

    bool pickSupport = false;
    if (!readjusted && !supportiveVertexToSSRAlgorithm.hasDefaultValue(v)) {
        PRINT_DEBUG("  Was a supportive vertex.");
        removeSupportiveVertex(v);
        pickSupport = true;
    }

    if (!doesAutoUpdate() && !readjusted) {
        for (auto &[ssrc, ssink] : supportiveSSRAlgorithms) {
            ssrc->onVertexRemove(v);
            ssink->onVertexRemove(v);
        }
    }
    if (pickSupport) {
        pickSupportVertices(false);
    }
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
void SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<
    DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>::onArcAdd(Arc *a)
{
    PRINT_DEBUG("An arc has been added: " << a);
    DynamicDiGraphAlgorithm::onArcAdd(a);

    if (!initialized || supportSize == 0.0) {
        return;
    }

    bool readjusted = false;
    if (reAdjust) {
        adjustmentCountUp++;
        if (adjustmentCountUp >= adjustAfter) {
            pickSupportVertices(true);
            adjustmentCountUp = 0;
            readjusted = true;
        }
    }

    if (!doesAutoUpdate() && !readjusted) {
        for (auto &[ssrc, ssink] : supportiveSSRAlgorithms) {
            ssrc->onArcAdd(a);
            ssink->onArcAdd(a);
        }
    }

}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
void SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<
    DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>::onArcRemove(Arc *a)
{
    PRINT_DEBUG("An arc is about to be deleted: " << a);
    DynamicDiGraphAlgorithm::onArcRemove(a);

    if (!initialized || supportSize == 0.0) {
        return;
    }


    bool readjusted = false;
    if (reAdjust) {
        adjustmentCountUp++;
        if (adjustmentCountUp >= adjustAfter) {
            pickSupportVertices(true);
            adjustmentCountUp = 0;
            readjusted = true;
        }
    }

    if (!doesAutoUpdate() && !readjusted) {
        for (auto &[ssrc, ssink] : supportiveSSRAlgorithms) {
            ssrc->onArcRemove(a);
            ssink->onArcRemove(a);
        }
    }
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
DynamicDiGraphAlgorithm::Profile
SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<
        DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>::getProfile() const
{
    auto profile = DynamicAllPairsReachabilityAlgorithm::getProfile();
    profile.push_back(std::make_pair(std::string("seed"),
                                     seed));
    profile.push_back(std::make_pair(std::string("min_supportive_ssr"),
                                     min_supportive_vertices));
    profile.push_back(std::make_pair(std::string("max_supportive_ssr"),
                                     max_supportive_vertices));
    profile.push_back(std::make_pair(std::string("num_trivial_queries"),
                                     num_trivial_queries));
    profile.push_back(std::make_pair(std::string("num_ssr_only_queries"),
                                     num_only_ssr_queries));
    profile.push_back(std::make_pair(std::string("num_support_only_queries_svt"),
                                     num_only_support_queries_svt));
    profile.push_back(std::make_pair(std::string("num_support_only_queries_vs"),
                                     num_only_support_queries_vs));
    profile.push_back(std::make_pair(std::string("num_support_only_queries_tv"),
                                     num_only_support_queries_tv));
    profile.push_back(std::make_pair(std::string("num_expensive_queries"),
                                     num_expensive_queries));
    profile.push_back(std::make_pair(std::string("num_adjustments"),
                                     num_adjustments));

    return profile;
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
void SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<
            DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>::onDiGraphSet()
{
    DynamicAllPairsReachabilityAlgorithm::onDiGraphSet();

    min_supportive_vertices = 0;
    max_supportive_vertices = 0;
    num_trivial_queries = 0;
    num_only_ssr_queries = 0;
    num_only_support_queries_svt = 0;
    num_only_support_queries_vs = 0;
    num_only_support_queries_tv = 0;
    num_expensive_queries = 0;
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
void SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<
            DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>::onDiGraphUnset()
{
    reset();
    DynamicAllPairsReachabilityAlgorithm::onDiGraphUnset();
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
bool SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<
    DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>::query(Vertex *s, Vertex *t)
{
    PRINT_DEBUG("Processing reachability query " << s << " -> " << t << "...");
    if (s == t) {
#ifdef COLLECT_PR_DATA
        num_trivial_queries++;
#endif
        PRINT_DEBUG("  Same vertices, trivially true.");
        return true;
    }
    if (diGraph->isSink(s) || diGraph->isSource(t)) {
#ifdef COLLECT_PR_DATA
        num_trivial_queries++;
#endif
        PRINT_DEBUG("  Source is sink or target is source, trivially false.");
        return false;
    }

    if (supportiveVertexToSSRAlgorithm[s].first) {
#ifdef COLLECT_PR_DATA
        num_only_ssr_queries++;
#endif
        PRINT_DEBUG("  Source is supportive vertex.");
        return supportiveVertexToSSRAlgorithm[s].first->query(t);
    }

    if (supportiveVertexToSSRAlgorithm[t].second) {
#ifdef COLLECT_PR_DATA
        num_only_ssr_queries++;
#endif
        PRINT_DEBUG("  Sink is supportive vertex.");
        return supportiveVertexToSSRAlgorithm[t].second->query(s);
    }

    for (const auto &[ssrc, ssink] : supportiveSSRAlgorithms) {
        auto vt = ssrc->query(t);
        auto sv = ssink->query(s);
        if (sv) {
            if (vt) {
#ifdef COLLECT_PR_DATA
                num_only_support_queries_svt++;
#endif
                PRINT_DEBUG("  Reachability established via supportive vertex "
                            << ssrc->getSource() <<  ".");
                return true;
            }
        } else if (ssink->query(t)) {
            // no path from s to v, but from t to v
#ifdef COLLECT_PR_DATA
                num_only_support_queries_tv++;
#endif
                PRINT_DEBUG("  Non-reachability established via supportive vertex "
                            << ssrc->getSource() <<  ".");
                return false;
        }
        if (!vt && ssrc->query(s)) {
            // no path from v to t, but from v to s
#ifdef COLLECT_PR_DATA
                num_only_support_queries_vs++;
#endif
                PRINT_DEBUG("  Non-reachability established via supportive vertex "
                            << ssrc->getSource() <<  ".");
                return false;
        }
    }

#ifdef COLLECT_PR_DATA
                num_expensive_queries++;
#endif
    // start two-way BFS
    FindDiPathAlgorithm<FastPropertyMap> fpa;
    fpa.setGraph(diGraph);
    fpa.setConstructPaths(false, false);
    fpa.setSourceAndTarget(s, t);
    // fpa.prepare() omitted for performance reasons
    fpa.run();
    return fpa.deliver();
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
std::vector<Arc *> SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<
    DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>
    ::queryPath(Vertex *, Vertex *)
{
    return std::vector<Arc*>();
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
void SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<
    DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>::reset()
{
    if (!initialized) {
        return;
    }

    for (auto &[ssrc, ssink] : supportiveSSRAlgorithms) {
        delete ssrc;
        delete ssink;
    }
    supportiveSSRAlgorithms.clear();
    supportiveVertexToSSRAlgorithm.resetAll();
    initialized = false;
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
void
SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<
    DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>
    ::removeSupportiveVertex(Vertex *v)
{
    auto ssrPair = supportiveVertexToSSRAlgorithm(v);
    assert(!supportiveSSRAlgorithms.empty());

    if (supportiveSSRAlgorithms.size() == 1) {
        assert(supportiveSSRAlgorithms.back() == ssrPair);
        supportiveSSRAlgorithms.clear();
    } else {
        if (supportiveSSRAlgorithms.back() == ssrPair) {
            supportiveSSRAlgorithms.pop_back();
        } else {
            auto pos = std::find(supportiveSSRAlgorithms.begin(),
                                 supportiveSSRAlgorithms.end(),
                                 ssrPair);
            assert(pos != supportiveSSRAlgorithms.end());
            *pos = supportiveSSRAlgorithms.back();
            supportiveSSRAlgorithms.pop_back();
        }
    }

    delete ssrPair.first;
    delete ssrPair.second;
    supportiveVertexToSSRAlgorithm.resetToDefault(v);
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
void SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<
    DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>::pickSupportVertices(
        bool adjust)
{

    if (supportSize <= 0) {
        return;
    }

    auto numSupportiveVertices =
               supportSize > 1.0
                    ? std::min(static_cast<DiGraph::size_type>(std::floor(supportSize)), diGraph->getSize())
                    : static_cast<DiGraph::size_type>(std::floor(supportSize * diGraph->getSize()));

    auto pickVertices = [&](auto num) {
        std::uniform_int_distribution<DiGraph::size_type> distVertex(0, diGraph->getSize() - 1);
        auto randomVertex = std::bind(distVertex, std::ref(gen));
        IncidenceListGraph *igraph = dynamic_cast<IncidenceListGraph*>(diGraph);

        if (num >= diGraph->getSize()) {
            diGraph->mapVertices([&](Vertex *v) {
                if (supportiveVertexToSSRAlgorithm(v)
                        == supportiveVertexToSSRAlgorithm.getDefaultValue()) {
                    createAndInitAlgorithm(v);
                    PRINT_DEBUG("  Created supportive SSR algorithm with source " << v);
                }
            });
        } else {
            decltype(num) i = 0;
            while (i < num) {
                auto v = igraph->vertexAt(randomVertex());
                assert(v);
                if (supportiveVertexToSSRAlgorithm(v)
                        == supportiveVertexToSSRAlgorithm.getDefaultValue()) {
                    createAndInitAlgorithm(v);
                    PRINT_DEBUG("  Created supportive SSR algorithm with source " << v);
                    i++;
                }
            }
        }
    };

    if (!adjust) {
        if (supportiveSSRAlgorithms.size() == numSupportiveVertices) {
            return;
        }
        if (supportiveSSRAlgorithms.size() < numSupportiveVertices) {
            pickVertices(numSupportiveVertices - supportiveSSRAlgorithms.size());
        }
    } else {
        if (supportiveSSRAlgorithms.empty()
                && supportiveVertexToSSRAlgorithm.size() < diGraph->getSize()) {
            supportiveVertexToSSRAlgorithm.resetAll(diGraph->getSize());
        }
        for (auto &[ssrc, ssink] : supportiveSSRAlgorithms) {
            supportiveVertexToSSRAlgorithm.resetToDefault(ssrc->getSource());
            delete ssrc;
            delete ssink;
        }
        supportiveSSRAlgorithms.clear();

        pickVertices(numSupportiveVertices);
        num_adjustments++;
    }

    if (supportiveSSRAlgorithms.size() < min_supportive_vertices) {
        min_supportive_vertices = supportiveSSRAlgorithms.size();
    }
    if (supportiveSSRAlgorithms.size() > max_supportive_vertices) {
        max_supportive_vertices = supportiveSSRAlgorithms.size();
    }
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
void SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<
    DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>
    ::createAndInitAlgorithm(Vertex *v)
{
    assert(supportiveVertexToSSRAlgorithm(v) == supportiveVertexToSSRAlgorithm.getDefaultValue());

    auto *ssrcAlgorithm = new DynamicSingleSourceAlgorithm(ssourceParameters);
    auto *ssinkAlgorithm = new DynamicSingleSinkAlgorithm(ssinkParameters);

    ssrcAlgorithm->setAutoUpdate(this->doesAutoUpdate());
    ssrcAlgorithm->setGraph(diGraph);
    ssrcAlgorithm->setSource(v);

    ssinkAlgorithm->setAutoUpdate(this->doesAutoUpdate());
    ssinkAlgorithm->setGraph(diGraph);
    ssinkAlgorithm->setSource(v);

    if (!ssrcAlgorithm->prepare() || !ssinkAlgorithm->prepare()) {
        throw DiGraphAlgorithmException(this, "Could not prepare SSR subalgorithms.");
    }
    ssrcAlgorithm->run();
    ssinkAlgorithm->run();

    auto ssrPair = SSRPair(ssrcAlgorithm, ssinkAlgorithm);
    supportiveVertexToSSRAlgorithm[v] = ssrPair;
    supportiveSSRAlgorithms.push_back(ssrPair);
}

}
