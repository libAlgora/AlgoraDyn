// license
#include "supportiveverticessloppysccsapralgorithm.h"

#include "algorithm.basic/tarjansccalgorithm.h"
#include "property/fastpropertymap.h"

//#define DEBUG_SVSSCC

#ifdef DEBUG_SVSSCC
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
SupportiveVerticesSloppySCCsAPRAlgorithm<
                DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>
    ::SupportiveVerticesSloppySCCsAPRAlgorithm(double supportSize, unsigned long adjustAfter)
    : Super(supportSize, adjustAfter)
{
    vertexToSCCRepresentative.setDefaultValue(nullptr);
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
SupportiveVerticesSloppySCCsAPRAlgorithm<
                DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>
    ::SupportiveVerticesSloppySCCsAPRAlgorithm(double supportSize, unsigned long adjustAfter,
                                const typename Super::SingleSourceParameterSet &ssourceParams,
                                const typename Super::SingleSinkParameterSet &ssinkParams)
    : Super(supportSize, adjustAfter, ssourceParams, ssinkParams)
{
    vertexToSCCRepresentative.setDefaultValue(nullptr);
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
SupportiveVerticesSloppySCCsAPRAlgorithm<
                DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>
    ::SupportiveVerticesSloppySCCsAPRAlgorithm(
        const typename Super::ParameterSet &params,
        const typename Super::SingleSourceParameterSet &ssourceParams,
        const typename Super::SingleSinkParameterSet &ssinkParams)
    : Super(params, ssourceParams, ssinkParams)
{
    vertexToSCCRepresentative.setDefaultValue(nullptr);
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
void
SupportiveVerticesSloppySCCsAPRAlgorithm<
                DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>
    ::run()
{
    if (this->initialized) {
        return;
    }

    PRINT_DEBUG("Initializing...");
    checkSCCs();
    this->initialized = true;
    PRINT_DEBUG("Initialization complete.");

    if (reAdjust) {
        this->adjustmentCountUp = 0;
    }
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
std::string
SupportiveVerticesSloppySCCsAPRAlgorithm<
                DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>
    ::getName() const noexcept
{
    DynamicSingleSourceAlgorithm ssrc(this->ssourceParameters);
    DynamicSingleSinkAlgorithm ssink(this->ssinkParameters);
    std::stringstream ss;
    ss << "Supportive Vertices with Sloppy SCCs All-Pairs Reachability Algorithm ("
       << this->supportSize << ", ";
    if (reAdjust) {
        ss << "adjust-after=" << this->adjustAfter << ", ";
    } else {
        ss << "no-adjust, ";
    }
    ss << ssrc.getName() << ", "
       << ssink.getName() << ")";
    return ss.str();
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
std::string
SupportiveVerticesSloppySCCsAPRAlgorithm<
                DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>
    ::getShortName() const noexcept
{
    DynamicSingleSourceAlgorithm ssrc(this->ssourceParameters);
    DynamicSingleSinkAlgorithm ssink(this->ssinkParameters);
    std::stringstream ss;
    ss << "SV-sSCC("
       << this->supportSize << ", ";
    if (reAdjust) {
        ss << "adjust-after=" << this->adjustAfter << ", ";
    } else {
        ss << "no-adjust, ";
    }
    ss << ssrc.getShortName() << ", "
       << ssink.getShortName() << ")";
    return ss.str();
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
void
SupportiveVerticesSloppySCCsAPRAlgorithm<
                DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>
    ::onVertexAdd(Vertex *v)
{
    PRINT_DEBUG("A vertex has been added: " << v);
    DynamicDiGraphAlgorithm::onVertexAdd(v);

    if (!this->initialized) {
        return;
    }

    // vertex is singleton, cannot be supportive
    assert(this->vertexToSCCRepresentative.hasDefaultValue(v));

    if (!this->doesAutoUpdate()) {
        for (auto &[ssrc, ssink] : this->supportiveSSRAlgorithms) {
            ssrc->onVertexAdd(v);
            ssink->onVertexAdd(v);
        }
    }

    if (reAdjust && this->adjustAfter > 0) {
        this->adjustmentCountUp++;
        if (this->adjustmentCountUp >= this->adjustAfter) {
            checkSCCs();
            this->adjustmentCountUp = 0;
        }
    }
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
void
SupportiveVerticesSloppySCCsAPRAlgorithm<
                DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>
    ::onVertexRemove(Vertex *v)
{
    PRINT_DEBUG("A vertex is about to be deleted: " << v);
    DynamicDiGraphAlgorithm::onVertexRemove(v);

    if (!this->initialized) {
        return;
    }

    if (!vertexToSCCRepresentative.hasDefaultValue(v)) {
        this->removeSupportiveVertex(v);
    }

    if (!this->doesAutoUpdate()) {
        for (auto &[ssrc, ssink] : this->supportiveSSRAlgorithms) {
            ssrc->onVertexRemove(v);
            ssink->onVertexRemove(v);
        }
    }

    if (reAdjust && this->adjustAfter > 0) {
        this->adjustmentCountUp++;
        if (this->adjustmentCountUp >= this->adjustAfter) {
            checkSCCs();
            this->adjustmentCountUp = 0;
        }
    }
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
void
SupportiveVerticesSloppySCCsAPRAlgorithm<
                DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>
    ::onArcAdd(Arc *a)
{
    PRINT_DEBUG("An arc has been added: " << a);
    DynamicDiGraphAlgorithm::onArcAdd(a);

    if (!this->initialized) {
        return;
    }

    if (!this->doesAutoUpdate()) {
        for (auto &[ssrc, ssink] : this->supportiveSSRAlgorithms) {
            ssrc->onArcAdd(a);
            ssink->onArcAdd(a);
        }
    }

    if (reAdjust && this->adjustAfter > 0) {
        this->adjustmentCountUp++;
        if (this->adjustmentCountUp >= this->adjustAfter) {
            checkSCCs();
            this->adjustmentCountUp = 0;
        }
    }
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
void
SupportiveVerticesSloppySCCsAPRAlgorithm<
                DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>
    ::onArcRemove(Arc *a)
{
    PRINT_DEBUG("An arc is about to be deleted: " << a);
    DynamicDiGraphAlgorithm::onArcRemove(a);

    if (!this->initialized) {
        return;
    }

    if (!this->doesAutoUpdate()) {
        for (auto &[ssrc, ssink] : this->supportiveSSRAlgorithms) {
            ssrc->onArcRemove(a);
            ssink->onArcRemove(a);
        }
    }

    if (reAdjust && this->adjustAfter > 0) {
        this->adjustmentCountUp++;
        if (this->adjustmentCountUp >= this->adjustAfter) {
            checkSCCs();
            this->adjustmentCountUp = 0;
        }
    }
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
void
SupportiveVerticesSloppySCCsAPRAlgorithm<
                DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>
    ::checkSCCs()
{
    TarjanSCCAlgorithm<FastPropertyMap> tarjan;
    tarjan.setGraph(this->diGraph);
    FastPropertyMap<DiGraph::size_type> sccs(0);
    sccs.resetAll(this->diGraph->getSize());
    tarjan.useModifiableProperty(&sccs);
    if (!tarjan.prepare()) {
        throw DiGraphAlgorithmException(this, "Could not prepare TarjanSCC algorithm.");
    }
    tarjan.run();
    auto numSccs = tarjan.deliver();

    if (numSccs > 1) {
        std::vector<Vertex*> sccIdToRepresentative(numSccs, nullptr);
        std::vector<Vertex*> repsToClear;
        for (auto &p : this->supportiveSSRAlgorithms) {
            auto rep = p.first->getSource();
            auto sccId = sccs(rep);
            assert(sccId < numSccs);
            if (!sccIdToRepresentative[sccId]) {
                sccIdToRepresentative[sccId] = rep;
            } else {
                repsToClear.push_back(rep);
            }
        }
        for (auto *v : repsToClear) {
            this->removeSupportiveVertex(v);
        }
        if (this->supportiveSSRAlgorithms.size() < numSccs) {
            std::vector<std::vector<Vertex*>> verticesInScc(numSccs);
            std::vector<Vertex*> candidateReps;
            std::vector<DiGraph::size_type> uncoveredSccs;
            this->diGraph->mapVertices(
                        [&sccIdToRepresentative,&sccs,&verticesInScc,&candidateReps,
                         &uncoveredSccs,this](Vertex *v) {
                auto sccId = sccs(v);
                if (sccIdToRepresentative[sccId]) {
                    vertexToSCCRepresentative[v] = sccIdToRepresentative[sccId];
                } else {
                    if (verticesInScc[sccId].empty()) {
                        // remember first vertex as potential representative
                        verticesInScc[sccId].push_back(v);
                        candidateReps.push_back(v);
                        uncoveredSccs.push_back(sccId);
                        sccIdToRepresentative[sccId] = v;
                    } else {
                        verticesInScc[sccId].push_back(v);
                    }
                }
            });
            // allows to use back() and pop_back()
            std::reverse(candidateReps.begin(), candidateReps.end());
            for (auto sccId : uncoveredSccs) {
                if (verticesInScc[sccId].size() >= this->minSccSize) {
                    auto rep = candidateReps.back();
                    createSupportVertex(rep);
                    for (auto *v : verticesInScc[sccId]) {
                        vertexToSCCRepresentative[v] = rep;
                    }
                }
                candidateReps.pop_back();
            }
        }
    } else {
        while (this->supportiveSSRAlgorithms.size() > 1) {
            this->removeSupportiveVertex(this->supportiveSSRAlgorithms.back().first->getSource());
        }
    }
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
void
SupportiveVerticesSloppySCCsAPRAlgorithm<
                DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>
    ::createSupportVertex(Vertex *v)
{
    this->createAndInitAlgorithm(v);

}

}
