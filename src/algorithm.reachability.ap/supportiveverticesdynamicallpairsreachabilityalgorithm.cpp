// license

#include "supportiveverticesdynamicallpairsreachabilityalgorithm.h"
#include "graph/digraph.h"
#include "graph.incidencelist/incidencelistgraph.h"
#include "graph.incidencelist/incidencelistvertex.h"
#include "algorithm.basic.traversal/breadthfirstsearch.h"

#include <sstream>
#include <random>

//#define DEBUG_SUPPVAPR

#ifdef DEBUG_SUPPVAPR
#include <iostream>
#define PRINT_DEBUG(msg) std::cerr << this->getShortName() << ": " << msg << std::endl;
#define IF_DEBUG(cmd) cmd;
#else
#define PRINT_DEBUG(msg) ((void)0)
#define IF_DEBUG(cmd)
#endif

namespace Algora {

template<typename DynamicSSRAlgorithm>
SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<DynamicSSRAlgorithm>
    ::SupportiveVerticesDynamicAllPairsReachabilityAlgorithm(double supportSizeRatio,
                                                             bool ssrSubtreeCheck,
                                                         const SSRParameterSet &ssrParams)
    : DynamicAllPairsReachabilityAlgorithm(),
      ssrParameters(ssrParams), initialized(false),
      supportSizeRatio(supportSizeRatio), twoWayStepSize(5U), ssrSubtreeCheck(ssrSubtreeCheck)
{
    if (supportSizeRatio == 0.0) {
        ssrSubtreeCheck = false;
    }
    supportiveVertexToSSRAlgorithm.setDefaultValue(nullptr);
}

template<typename DynamicSSRAlgorithm>
SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<DynamicSSRAlgorithm>
    ::SupportiveVerticesDynamicAllPairsReachabilityAlgorithm(double supportSizeRatio,
                                                             bool ssrSubtreeCheck)
    : SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<DynamicSSRAlgorithm>(
          supportSizeRatio, ssrSubtreeCheck, {})
{ }

template<typename DynamicSSRAlgorithm>
SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<DynamicSSRAlgorithm>
    ::~SupportiveVerticesDynamicAllPairsReachabilityAlgorithm()
{
    reset();
}

template<typename DynamicSSRAlgorithm>
void SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<DynamicSSRAlgorithm>::run()
{
    if (initialized) {
        return;
    }

    PRINT_DEBUG("Initializing...");
    if (supportSizeRatio > 0) {

        if (supportiveVertexToSSRAlgorithm.size() < diGraph->getSize()) {
            supportiveVertexToSSRAlgorithm.resetAll(diGraph->getSize());
        }

        std::random_device rd;
        auto seed = rd();
        std::mt19937_64 gen;
        gen.seed(seed);
        std::uniform_int_distribution<DiGraph::size_type> distVertex(
                    0, diGraph->getSize() - 1);
        auto randomVertex = std::bind(distVertex, std::ref(gen));
        auto numSupportiveVertices = std::round(supportSizeRatio * diGraph->getSize());
        IncidenceListGraph *igraph = dynamic_cast<IncidenceListGraph*>(diGraph);
        while (supportiveSSRAlgorithms.size() < numSupportiveVertices) {
            auto v = igraph->vertexAt(randomVertex());
            assert(v);
            if (supportiveVertexToSSRAlgorithm(v) == nullptr) {
                createAndInitAlgorithm(v);
                PRINT_DEBUG("  Created supportive SSR algorithm with source " << v);
            }
        }
    }
    min_supportive_vertices = supportiveSSRAlgorithms.size();
    max_supportive_vertices = min_supportive_vertices;
    initialized = true;
    PRINT_DEBUG("Initialization complete.");
}

template<typename DynamicSSRAlgorithm>
std::string SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<DynamicSSRAlgorithm>::getName()
const noexcept
{
    DynamicSSRAlgorithm a(ssrParameters);
    std::stringstream ss;
    ss << "Single-Source-Supported All-Pairs Reachability Algorithm ("
       << supportSizeRatio << ", "
       << (ssrSubtreeCheck ? "subtree check" : "no subtree check") << ", "
       << a.getName() << ")";
    return ss.str();
}

template<typename DynamicSSRAlgorithm>
std::string SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<DynamicSSRAlgorithm>
    ::getShortName() const noexcept
{
    DynamicSSRAlgorithm a(ssrParameters);
    std::stringstream ss;
    ss << "SSR-Based APR("
       << supportSizeRatio << ", "
       << (ssrSubtreeCheck ? "SSRSUB" : "NSSRSUB") << ", "
       <<  a.getShortName() << ")";
    return ss.str();
}

template<typename DynamicSSRAlgorithm>
std::string
SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<DynamicSSRAlgorithm>::getProfilingInfo()
const
{
    std::stringstream ss;
#ifdef COLLECT_PR_DATA
    ss << "#vertices considered:         " << pr_consideredVertices << std::endl;
    ss << "#arcs considered:             " << pr_consideredArcs << std::endl;
    ss << "#supportive vertices (min):   " << min_supportive_vertices << std::endl;
    ss << "#supportive vertices (max):   " << max_supportive_vertices << std::endl;
    ss << "#supportive vertex hits:      " << supportive_ssr_hits << std::endl;
    ss << "#ssr subtree checks:          " << ssr_subtree_checks << std::endl;
    ss << "#ssr subtree hits:            " << ssr_subtree_hits << std::endl;
    ss << "total #steps forward bfs:     " << forward_bfs_total_steps << std::endl;
    ss << "total #steps backward bfs:    " << backward_bfs_total_steps << std::endl;
    ss << "#query resumes:               " << num_query_resume<< std::endl;
    ss << "#trivial queries:             " << num_trivial_queries << std::endl;
    ss << "#SSR-only queries:            " << num_only_ssr_queries << std::endl;
#endif
    return ss.str();
}

template<typename DynamicSSRAlgorithm>
void SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<DynamicSSRAlgorithm>
    ::onVertexAdd(Vertex *v)
{
    PRINT_DEBUG("A vertex has been added: " << v);
    DynamicDiGraphAlgorithm::onVertexAdd(v);

    if (!initialized || supportSizeRatio == 0) {
        return;
    }

    if (!doesAutoUpdate()) {
        for (auto *a : supportiveSSRAlgorithms) {
            a->onVertexAdd(v);
        }
    }

    if (supportiveSSRAlgorithms.size() < std::round(supportSizeRatio * diGraph->getSize())) {
        createAndInitAlgorithm(v);
        PRINT_DEBUG("  Becomes new supportive vertex.");
    }

    if (max_supportive_vertices < supportiveSSRAlgorithms.size()) {
        max_supportive_vertices = supportiveSSRAlgorithms.size();
    }
}

template<typename DynamicSSRAlgorithm>
void SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<DynamicSSRAlgorithm>
    ::onVertexRemove(Vertex *v)
{
    PRINT_DEBUG("A vertex is about to be deleted: " << v);
    DynamicDiGraphAlgorithm::onVertexRemove(v);

    if (!initialized || supportSizeRatio == 0) {
        return;
    }

    if (!doesAutoUpdate()) {
        for (auto *a : supportiveSSRAlgorithms) {
            a->onVertexRemove(v);
        }
    }

    auto *ssrAlgorithm = supportiveVertexToSSRAlgorithm(v);
    if (ssrAlgorithm) {
        assert(!supportiveSSRAlgorithms.empty());
        PRINT_DEBUG("  Was a supportive vertex.");

        if (supportiveSSRAlgorithms.size() == 1) {
            assert(supportiveSSRAlgorithms.back() == ssrAlgorithm);
            supportiveSSRAlgorithms.clear();
        } else {
            if (supportiveSSRAlgorithms.back() == ssrAlgorithm) {
                supportiveSSRAlgorithms.pop_back();
            } else {
                auto pos = std::find(supportiveSSRAlgorithms.begin(),
                                     supportiveSSRAlgorithms.end(),
                                     ssrAlgorithm);
                assert(pos != supportiveSSRAlgorithms.end());
                *pos = supportiveSSRAlgorithms.back();
                supportiveSSRAlgorithms.pop_back();
            }
        }

        delete ssrAlgorithm;
        supportiveVertexToSSRAlgorithm.resetToDefault(v);

        if (supportiveSSRAlgorithms.size() < std::round(supportSizeRatio * diGraph->getSize())) {
            Vertex *replacementVertex = nullptr;
            diGraph->mapOutgoingArcsUntil(v, [&replacementVertex, this](Arc *a) {
                if (!a->isLoop() && !supportiveVertexToSSRAlgorithm(a->getHead())) {
                    replacementVertex = a->getHead();
                }
            }, [replacementVertex](const Arc*) { return replacementVertex != nullptr; });
            if (!replacementVertex) {
                diGraph->mapIncomingArcsUntil(v, [&replacementVertex, this](Arc *a) {
                    if (!a->isLoop() && !supportiveVertexToSSRAlgorithm(a->getTail())) {
                        replacementVertex = a->getTail();
                    }
                }, [replacementVertex](const Arc*) { return replacementVertex != nullptr; });
            }
            // TODO: if still no replacement found, try other strategy
        }
    }
    if (min_supportive_vertices > supportiveSSRAlgorithms.size()) {
        min_supportive_vertices = supportiveSSRAlgorithms.size();
    }
}

template<typename DynamicSSRAlgorithm>
void SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<DynamicSSRAlgorithm>::onArcAdd(Arc *a)
{
    PRINT_DEBUG("An arc has been added: " << a);
    DynamicDiGraphAlgorithm::onArcAdd(a);

    if (!initialized || supportSizeRatio == 0) {
        return;
    }

    if (!doesAutoUpdate()) {
        for (auto *ssrAlgorithm : supportiveSSRAlgorithms) {
            ssrAlgorithm->onArcAdd(a);
        }
    }
}

template<typename DynamicSSRAlgorithm>
void SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<DynamicSSRAlgorithm>::onArcRemove(
        Arc *a)
{
    PRINT_DEBUG("An arc is about to be deleted: " << a);
    DynamicDiGraphAlgorithm::onArcRemove(a);

    if (!initialized || supportSizeRatio == 0) {
        return;
    }

    if (!doesAutoUpdate()) {
        for (auto *ssrAlgorithm : supportiveSSRAlgorithms) {
            ssrAlgorithm->onArcRemove(a);
        }
    }
}

template<typename DynamicSSRAlgorithm>
DynamicDiGraphAlgorithm::Profile
SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<DynamicSSRAlgorithm>::getProfile() const
{
    auto profile = DynamicAllPairsReachabilityAlgorithm::getProfile();
    profile.push_back(std::make_pair(std::string("min_supportive_ssr"),
                                     min_supportive_vertices));
    profile.push_back(std::make_pair(std::string("max_supportive_ssr"),
                                     max_supportive_vertices));
    profile.push_back(std::make_pair(std::string("supportive_ssr_hits"),
                                     supportive_ssr_hits));
    profile.push_back(std::make_pair(std::string("ssr_subtree_checks"),
                                     ssr_subtree_checks));
    profile.push_back(std::make_pair(std::string("ssr_subtree_hits"),
                                     ssr_subtree_hits));
    profile.push_back(std::make_pair(std::string("forward_bfs_total_steps"),
                                     forward_bfs_total_steps));
    profile.push_back(std::make_pair(std::string("backward_bfs_total_steps"),
                                     backward_bfs_total_steps));
    profile.push_back(std::make_pair(std::string("num_query_resume"),
                                     num_query_resume));
    profile.push_back(std::make_pair(std::string("num_trivial_queries"),
                                     num_trivial_queries));
    profile.push_back(std::make_pair(std::string("num_ssr_only_queries"),
                                     num_only_ssr_queries));

    return profile;
}

template<typename DynamicSSRAlgorithm>
void SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<DynamicSSRAlgorithm>::onDiGraphSet()
{
    DynamicAllPairsReachabilityAlgorithm::onDiGraphSet();

    min_supportive_vertices = 0;
    max_supportive_vertices = 0;
    supportive_ssr_hits = 0;
    ssr_subtree_checks = 0;
    ssr_subtree_hits = 0;
    forward_bfs_total_steps = 0;
    backward_bfs_total_steps = 0;
    num_trivial_queries = 0;
    num_only_ssr_queries = 0;
    num_query_resume = 0;
}

template<typename DynamicSSRAlgorithm>
void SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<DynamicSSRAlgorithm>::onDiGraphUnset()
{
    reset();
    DynamicAllPairsReachabilityAlgorithm::onDiGraphUnset();
}

template<typename DynamicSSRAlgorithm>
bool SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<DynamicSSRAlgorithm>::query(Vertex *s,
                                                                                        Vertex *t)
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

    if (supportiveVertexToSSRAlgorithm[s]) {
#ifdef COLLECT_PR_DATA
        num_only_ssr_queries++;
#endif
        PRINT_DEBUG("  Source is supportive vertex.");
        return supportiveVertexToSSRAlgorithm[s]->query(t);
    }

    assert(twoWayStepSize > 0);
    bool reachable = false;
    std::vector<DynamicSSRAlgorithm*> suppAlgorithms;
    auto forwardStop = twoWayStepSize;
    auto backwardStop = twoWayStepSize;

    BreadthFirstSearch<FastPropertyMap,false> forwardBfs(false, false);
    forwardBfs.setStartVertex(s);
    forwardBfs.setGraph(diGraph);

    BreadthFirstSearch<FastPropertyMap,false,true> backwardBfs(false, false);
    backwardBfs.setStartVertex(t);
    backwardBfs.setGraph(diGraph);

    forwardBfs.setArcStopCondition([&reachable,&backwardBfs,this,t](const Arc *a) {
#ifdef COLLECT_PR_DATA
        this->forward_bfs_total_steps++;
#endif
        PRINT_DEBUG("  Forward-looking at " << a);
        if (backwardBfs.vertexDiscovered(a->getHead()) || a->getHead() == t) {
            reachable = true;
        }
        return reachable;
    });
    backwardBfs.setArcStopCondition([&reachable,&forwardBfs,this,s](const Arc *a) {
#ifdef COLLECT_PR_DATA
        this->backward_bfs_total_steps++;
#endif
        PRINT_DEBUG("  Backward-looking at " << a);
        if (forwardBfs.vertexDiscovered(a->getTail()) || a->getTail() == s) {
            reachable = true;
        }
        return reachable;
    });

    if (supportiveSSRAlgorithms.size() > 0) {
        //forwardBfs.onArcDiscover(
        forwardBfs.onVertexDiscover(
                    [&t,&reachable,&suppAlgorithms,this]
                    //(const Arc *a) {
                    (const Vertex *head) {
            PRINT_DEBUG("  Forward-discovered vertex " << head << ".");
            auto suppAlg = supportiveVertexToSSRAlgorithm(head);
            if (suppAlg) {
#ifdef COLLECT_PR_DATA
                this->supportive_ssr_hits++;
#endif
                bool reach = suppAlg->query(t);
                if (!reach) {
                    if (ssrSubtreeCheck) {
                        suppAlgorithms.push_back(suppAlg);
                    }
                    PRINT_DEBUG("    Is supportive vertex, but can't reach target.");
                } else {
                    reachable = true;
                    PRINT_DEBUG("    Is supportive vertex, CAN reach target!");
                }
                // stop anyway
                return false;
            }
            if (ssrSubtreeCheck) {
                for (auto *alg : suppAlgorithms) {
#ifdef COLLECT_PR_DATA
                    this->ssr_subtree_checks++;
#endif
                    if (alg->query(head)) {
#ifdef COLLECT_PR_DATA
                        this->ssr_subtree_hits++;
#endif
                        PRINT_DEBUG("    Is in reachability tree rooted at " << alg->getSource() << ".");
                        // source of alg could not reach t, but head, so head cannot reach t
                        return false;
                    }
                }
            }
            // can't say anything about reachability
            return true;
        });
    }

    auto stepSize = twoWayStepSize;
    forwardBfs.setVertexStopCondition(
                [&forwardBfs,&forwardStop,&stepSize](const Vertex *) {
        if (forwardBfs.getMaxBfsNumber() >= forwardStop) {
            forwardStop += stepSize;
            return true;
        }
        return false;
    });

    backwardBfs.setVertexStopCondition(
                [&backwardBfs,&backwardStop,&stepSize](const Vertex *) {
        if (backwardBfs.getMaxBfsNumber() >= backwardStop) {
            backwardStop += stepSize;
            return true;
        }
        return false;
    });

    forwardBfs.prepare();
    backwardBfs.prepare();

    PRINT_DEBUG(" Running forward search...");
    forwardBfs.run();
    if (!reachable && !forwardBfs.isExhausted() && forwardBfs.getMaxLevel() < diGraph->getSize()) {
        PRINT_DEBUG(" Running backward search...");
        backwardBfs.run();
    }

    while (!reachable && !forwardBfs.isExhausted() && !backwardBfs.isExhausted()
           && forwardBfs.getMaxLevel() + backwardBfs.getMaxLevel() < diGraph->getSize()) {
#ifdef COLLECT_PR_DATA
        this->num_query_resume++;
#endif
        PRINT_DEBUG(" Resuming forward search...");
        forwardBfs.resume();
        if (!reachable && !forwardBfs.isExhausted() && !backwardBfs.isExhausted()
                && forwardBfs.getMaxLevel() + backwardBfs.getMaxLevel() < diGraph->getSize()) {
            PRINT_DEBUG(" Resuming backward search...");
            backwardBfs.resume();
        }
    }
    PRINT_DEBUG("  Answering query with " << reachable << ".");
    return reachable;
}

template<typename DynamicSSRAlgorithm>
std::vector<Arc *> SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<DynamicSSRAlgorithm>
    ::queryPath(Vertex *, Vertex *)
{
    return std::vector<Arc*>();
}

template<typename DynamicSSRAlgorithm>
void SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<DynamicSSRAlgorithm>::reset()
{
    if (!initialized) {
        return;
    }

    for (auto *a : supportiveVertexToSSRAlgorithm) {
        if (a) {
            delete a;
        }
    }
    supportiveSSRAlgorithms.clear();
    supportiveVertexToSSRAlgorithm.resetAll();
    initialized = false;
}

template<typename DynamicSSRAlgorithm>
void SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<DynamicSSRAlgorithm>
    ::createAndInitAlgorithm(Vertex *v)
{
    assert(!supportiveVertexToSSRAlgorithm(v));

    auto *ssrAlgorithm = new DynamicSSRAlgorithm(ssrParameters);
    ssrAlgorithm->setAutoUpdate(this->doesAutoUpdate());
    ssrAlgorithm->setGraph(diGraph);
    ssrAlgorithm->setSource(v);
    if (!ssrAlgorithm->prepare()) {
        throw DiGraphAlgorithmException(this, "Could not prepare SSR subalgorithm.");
    }
    ssrAlgorithm->run();
    supportiveVertexToSSRAlgorithm[v] = ssrAlgorithm;
    supportiveSSRAlgorithms.push_back(ssrAlgorithm);
}

}
