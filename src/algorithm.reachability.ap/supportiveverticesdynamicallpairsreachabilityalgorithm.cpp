// license
#include "supportiveverticesdynamicallpairsreachabilityalgorithm.h"
#include "graph/digraph.h"
#include "graph.incidencelist/incidencelistgraph.h"
#include "graph.incidencelist/incidencelistvertex.h"
#include <sstream>
#include <random>

namespace Algora {

template<typename DynamicSSRAlgorithm>
SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<DynamicSSRAlgorithm>
    ::SupportiveVerticesDynamicAllPairsReachabilityAlgorithm(double supportSizeRatio)
    : DynamicAllPairsReachabilityAlgorithm(), supportSizeRatio(supportSizeRatio)
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
void SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<DynamicSSRAlgorithm>::onDiGraphUnset()
{
    reset();
    DynamicAllPairsReachabilityAlgorithm::onDiGraphUnset();
}

template<typename DynamicSSRAlgorithm>
bool SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<DynamicSSRAlgorithm>::query(Vertex *s,
                                                                                        Vertex *t)
{
    return false;
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
    ssrAlgorithm->setGraph(diGraph);
    ssrAlgorithm->setSource(v);
    ssrAlgorithm->setAutoUpdate(this->doesAutoUpdate());
    if (!ssrAlgorithm->prepare()) {
        throw DiGraphAlgorithmException(this, "Could not prepare SSR subalgorithm.");
    }
    ssrAlgorithm->run();
    supportiveVertexToSSRAlgorithm[v] = ssrAlgorithm;
    supportiveSSRAlgorithms.push_back(ssrAlgorithm);
}

}
