// license

#include "supportiveverticesdynamicallpairsreachabilityalgorithm.h"
#include "graph/digraph.h"
#include "graph.incidencelist/incidencelistgraph.h"
#include "graph.incidencelist/incidencelistvertex.h"
#include "algorithm.basic.traversal/breadthfirstsearch.h"

#include <sstream>
#include <random>

namespace Algora {

template<typename DynamicSSRAlgorithm>
SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<DynamicSSRAlgorithm>
    ::SupportiveVerticesDynamicAllPairsReachabilityAlgorithm(double supportSizeRatio)
    : DynamicAllPairsReachabilityAlgorithm(),
      initialized(false), supportSizeRatio(supportSizeRatio), twoWayStepSize(5U)
{
    supportiveVertexToSSRAlgorithm.setDefaultValue(nullptr);
}

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
        }
    }
    initialized = true;
}

template<typename DynamicSSRAlgorithm>
std::string SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<DynamicSSRAlgorithm>::getName()
const noexcept
{
    DynamicSSRAlgorithm a;
    std::stringstream ss;
    ss << "Single-Source-Supported All-Pairs Reachability Algorithm ("
       << supportSizeRatio << ", "
       << a.getName() << ")";
    return ss.str();
}

template<typename DynamicSSRAlgorithm>
std::string SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<DynamicSSRAlgorithm>
    ::getShortName() const noexcept
{
    DynamicSSRAlgorithm a;
    std::stringstream ss;
    ss << "SSR-Based APR("
       << supportSizeRatio << ", "
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
    ss << DynamicAllPairsReachabilityAlgorithm::getProfilingInfo();
    ss << "#supportive vertices: " << supportiveSSRAlgorithms.size() << std::endl;
    ss << "#supportive vertex hits: " <<  supportive_ssr_hits << std::endl;
    ss << "#known unreachable hits: " <<  known_unreachable_hits << std::endl;
    ss << "#ssr subtree checks: " <<  ssr_subtree_checks << std::endl;
    ss << "total #steps forward bfs: " <<  forward_bfs_total_steps << std::endl;
    ss << "total #steps backward bfs: " <<  backward_bfs_total_steps << std::endl;
    ss << "#query resumes: " <<  num_query_resume<< std::endl;
    ss << "#trivial queries: " <<  num_trivial_queries << std::endl;
    ss << "#SSR-only queries: " <<  num_only_ssr_queries << std::endl;
#endif
    return ss.str();
}

template<typename DynamicSSRAlgorithm>
void SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<DynamicSSRAlgorithm>
    ::onVertexAdd(Vertex *v)
{
    DynamicDiGraphAlgorithm::onVertexAdd(v);

    if (!initialized) {
        return;
    }

    if (!doesAutoUpdate()) {
        for (auto *a : supportiveSSRAlgorithms) {
            a->onVertexAdd(v);
        }
    }

    if (supportiveSSRAlgorithms.size() < std::round(supportSizeRatio * diGraph->getSize())) {
        createAndInitAlgorithm(v);
    }
}

template<typename DynamicSSRAlgorithm>
void SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<DynamicSSRAlgorithm>
    ::onVertexRemove(Vertex *v)
{
    DynamicDiGraphAlgorithm::onVertexRemove(v);

    if (!initialized) {
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
}

template<typename DynamicSSRAlgorithm>
void SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<DynamicSSRAlgorithm>::onArcAdd(Arc *a)
{
    DynamicDiGraphAlgorithm::onArcAdd(a);

    if (!initialized) {
        return;
    }

    if (!doesAutoUpdate()) {
        for (auto *ssrAlgorithm : supportiveSSRAlgorithms) {
            ssrAlgorithm->onArcAdd(a);
        }
    }
}

template<typename DynamicSSRAlgorithm>
void SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<DynamicSSRAlgorithm>::onArcRemove(Arc *a)
{
    DynamicDiGraphAlgorithm::onArcAdd(a);

    if (!initialized) {
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
    profile.push_back(std::make_pair(std::string("num_supportive_ssr"),
                                     supportiveSSRAlgorithms.size()));
    profile.push_back(std::make_pair(std::string("supportive_ssr_hits"),
                                     supportive_ssr_hits));
    profile.push_back(std::make_pair(std::string("known_unreachable_hits"),
                                     known_unreachable_hits));
    profile.push_back(std::make_pair(std::string("ssr_subtree_checks"),
                                     ssr_subtree_checks));
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

    supportive_ssr_hits = 0;
    known_unreachable_hits = 0;
    ssr_subtree_checks = 0;
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
    if (s == t) {
#ifdef COLLECT_PR_DATA
        num_trivial_queries++;
#endif
        return true;
    }
    if (diGraph->isSink(s) || diGraph->isSource(t)) {
#ifdef COLLECT_PR_DATA
        num_trivial_queries++;
#endif
        return false;
    }

    if (supportiveVertexToSSRAlgorithm[s]) {
#ifdef COLLECT_PR_DATA
        num_only_ssr_queries++;
#endif
        return supportiveVertexToSSRAlgorithm[s]->query(t);
    }

    assert(twoWayStepSize > 0);
    bool reachable = false;
    std::vector<DynamicSSRAlgorithm*> suppAlgorithms;
    FastPropertyMap<bool> knownUnreachableFrom(false, "", diGraph->getSize());
    auto forwardStop = twoWayStepSize;
    auto backwardStop = twoWayStepSize;

    BreadthFirstSearch<FastPropertyMap,false> forwardBfs(false, false);
    forwardBfs.setStartVertex(s);
    forwardBfs.setGraph(diGraph);

    BreadthFirstSearch<FastPropertyMap,false,true> backwardBfs(false, false);
    backwardBfs.setStartVertex(t);
    backwardBfs.setGraph(diGraph);

    forwardBfs.setArcStopCondition([&reachable,&backwardBfs,this](const Arc *a) {
#ifdef COLLECT_PR_DATA
        this->forward_bfs_total_steps++;
#endif
        if (backwardBfs.vertexDiscovered(a->getHead())) {
            reachable = true;
        }
        return reachable;
    });
    backwardBfs.setArcStopCondition([&reachable,&forwardBfs,this](const Arc *a) {
#ifdef COLLECT_PR_DATA
        this->backward_bfs_total_steps++;
#endif
        if (forwardBfs.vertexDiscovered(a->getTail())) {
            reachable = true;
        }
        return reachable;
    });

    forwardBfs.onArcDiscover(
                [&knownUnreachableFrom,&t,&reachable,&suppAlgorithms,this](const Arc *a) {
        auto head = a->getHead();
        if (head == t) {
            reachable = true;
            // stop
            return false;
        }
        if (knownUnreachableFrom(head)) {
#ifdef COLLECT_PR_DATA
        this->known_unreachable_hits++;
#endif
            return false;
        }
        auto suppAlg = supportiveVertexToSSRAlgorithm(head);
        if (suppAlg) {
#ifdef COLLECT_PR_DATA
        this->supportive_ssr_hits++;
#endif
            bool reach = suppAlg->query(t);
            if (!reach) {
                knownUnreachableFrom[head] = true;
                suppAlgorithms.push_back(suppAlg);
            } else {
                reachable = true;
            }
            // stop anyway
            return false;
        }
        for (auto *alg : suppAlgorithms) {
#ifdef COLLECT_PR_DATA
        this->ssr_subtree_checks++;
#endif
            if (alg->query(head)) {
                // source of alg could not reach t, but head, so head cannot reach t
                knownUnreachableFrom[head] = true;
                return false;
            }
        }
        // can't say anything about reachability
        return true;
    });

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

    forwardBfs.run();
    backwardBfs.run();

    while (!reachable && !forwardBfs.isExhausted() && !backwardBfs.isExhausted()
           && forwardBfs.getMaxLevel() + backwardBfs.getMaxLevel() < diGraph->getSize()) {
#ifdef COLLECT_PR_DATA
        this->num_query_resume++;
#endif
        forwardBfs.resume();
        backwardBfs.resume();
    }
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

    auto *ssrAlgorithm = new DynamicSSRAlgorithm;
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
