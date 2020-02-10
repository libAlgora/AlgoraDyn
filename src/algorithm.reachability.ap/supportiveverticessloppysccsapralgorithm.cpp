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

    PRINT_DEBUG("Initializing...")
    checkSCCs();
    this->initialized = true;
    PRINT_DEBUG("Initialization complete.")

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
std::string
SupportiveVerticesSloppySCCsAPRAlgorithm<
                DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>
    ::getProfilingInfo() const
{
    std::stringstream ss;
    ss << "Seed:                         " << this->seed << std::endl;
#ifdef COLLECT_PR_DATA
    ss << "#vertices considered:         " << this->pr_consideredVertices << std::endl;
    ss << "#arcs considered:             " << this->pr_consideredArcs << std::endl;
    ss << "#supportive vertices (min):   " << this->min_supportive_vertices << std::endl;
    ss << "#supportive vertices (max):   " << this->max_supportive_vertices << std::endl;
    ss << "#trivial queries:             " << this->num_trivial_queries << std::endl;
    ss << "#SSR-only queries:            " << this->num_only_ssr_queries << std::endl;
    ss << "#SCC queries (via s-rep):     " << this->num_scc_via_srep_queries << std::endl;
    ss << "#SCC queries (via t-rep):     " << this->num_scc_via_trep_queries << std::endl;
    ss << "#Support-only queries (svt):  " << this->num_only_support_queries_svt << std::endl;
    ss << "#Support-only queries (vs):   " << this->num_only_support_queries_vs << std::endl;
    ss << "#Support-only queries (tv):   " << this->num_only_support_queries_tv << std::endl;
    ss << "#Expensive queries:           " << this->num_expensive_queries << std::endl;
    ss << "#Adjustments:                 " << this->num_adjustments << std::endl;
#endif
    return ss.str();

}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
void
SupportiveVerticesSloppySCCsAPRAlgorithm<
                DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>
    ::onVertexAdd(Vertex *v)
{
    PRINT_DEBUG("A vertex has been added: " << v)
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
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
void
SupportiveVerticesSloppySCCsAPRAlgorithm<
                DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>
    ::onVertexRemove(Vertex *v)
{
    PRINT_DEBUG("A vertex is about to be deleted: " << v)
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
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
void
SupportiveVerticesSloppySCCsAPRAlgorithm<
                DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>
    ::onArcAdd(Arc *a)
{
    PRINT_DEBUG("An arc has been added: " << a)
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
    PRINT_DEBUG("An arc is about to be deleted: " << a)
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
typename SupportiveVerticesSloppySCCsAPRAlgorithm<
                DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>::Super::Profile
SupportiveVerticesSloppySCCsAPRAlgorithm<
                DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>
    ::getProfile() const
{
    auto profile = Super::getProfile();

    profile.push_back(std::make_pair(std::string("num_scc_via_srep_queries"),
                                     num_scc_via_srep_queries));
    profile.push_back(std::make_pair(std::string("num_scc_via_trep_queries"),
                                     num_scc_via_trep_queries));
    return profile;
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
bool
SupportiveVerticesSloppySCCsAPRAlgorithm<
                DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>
    ::query(Vertex *s, Vertex *t)
{
    PRINT_DEBUG("Processing reachability query " << s << " -> " << t << "...")
    if (s == t) {
#ifdef COLLECT_PR_DATA
        this->num_trivial_queries++;
#endif
        PRINT_DEBUG("  Same vertices, trivially true.")
        return true;
    }
    if (this->diGraph->isSink(s) || this->diGraph->isSource(t)) {
#ifdef COLLECT_PR_DATA
        this->num_trivial_queries++;
#endif
        PRINT_DEBUG("  Source is sink or target is source, trivially false.")
        return false;
    }

    if (this->supportiveVertexToSSRAlgorithm[s].first) {
#ifdef COLLECT_PR_DATA
        this->num_only_ssr_queries++;
#endif
        PRINT_DEBUG("  Source is supportive vertex.")
        return this->supportiveVertexToSSRAlgorithm[s].first->query(t);
    }

    if (this->supportiveVertexToSSRAlgorithm[t].second) {
#ifdef COLLECT_PR_DATA
        this->num_only_ssr_queries++;
#endif
        PRINT_DEBUG("  Sink is supportive vertex.")
        return this->supportiveVertexToSSRAlgorithm[t].second->query(s);
    }

    PRINT_DEBUG("  Trying to use SCC information...")
    // use SCC info
    auto getRep = [this](Vertex *v) -> Vertex* {
        PRINT_DEBUG("  Looking up representative for " << v << "...")
        auto rep = this->vertexToSCCRepresentative(v);
        PRINT_DEBUG("    v2r map yields " << rep)
        if (!rep) {
            return nullptr;
        }
        const auto &[rSrc, rSink] = this->supportiveVertexToSSRAlgorithm(rep);
        // check!
        if (!rSrc || !rSink)  {
            // rep might have been deleted since last update
            this->vertexToSCCRepresentative[v] = nullptr;
            return nullptr;
        }

        PRINT_DEBUG("    Checking whether information is up-to-date...")
        if (rSrc->query(v) && rSink->query(v)) {
            PRINT_DEBUG("      OK.")
            return rep;
        }
        PRINT_DEBUG("      FAILED.")
        this->vertexToSCCRepresentative[v] = nullptr;
        return nullptr;
    };

    auto sRep = getRep(s);
    PRINT_DEBUG("  Source has representative " << sRep)
    if (sRep) {
        // either s ->* sRep ->* t  => TRUE
        // or sRep ->* s, but sRep -/>* t  => FALSE
#ifdef COLLECT_PR_DATA
        num_scc_via_srep_queries++;
#endif
        PRINT_DEBUG("  Answering query via source rep.")
        return this->supportiveVertexToSSRAlgorithm(sRep).first->query(t);
    }

    auto tRep = getRep(t);
    PRINT_DEBUG("  Target has representative " << tRep)
    if (tRep) {
        // either s ->* tRep ->* t  => TRUE
        // or t ->* tRep, but s -/>* tRep  => FALSE
#ifdef COLLECT_PR_DATA
        num_scc_via_trep_queries++;
#endif
        PRINT_DEBUG("  Answering query via target rep.")
        return this->supportiveVertexToSSRAlgorithm(tRep).second->query(s);
    }

    // fall back to standard SV algorithm
    for (const auto &[ssrc, ssink] : this->supportiveSSRAlgorithms) {
        auto vt = ssrc->query(t);
        auto sv = ssink->query(s);
        if (sv) {
            if (vt) {
#ifdef COLLECT_PR_DATA
                this->num_only_support_queries_svt++;
#endif
                PRINT_DEBUG("  Reachability established via supportive vertex "
                            << ssrc->getSource() <<  ".")
                return true;
            }
        } else if (ssink->query(t)) {
            // no path from s to v, but from t to v
#ifdef COLLECT_PR_DATA
                this->num_only_support_queries_tv++;
#endif
                PRINT_DEBUG("  Non-reachability established via supportive vertex "
                            << ssrc->getSource() <<  ".")
                return false;
        }
        if (!vt && ssrc->query(s)) {
            // no path from v to t, but from v to s
#ifdef COLLECT_PR_DATA
                this->num_only_support_queries_vs++;
#endif
                PRINT_DEBUG("  Non-reachability established via supportive vertex "
                            << ssrc->getSource() <<  ".")
                return false;
        }
    }

#ifdef COLLECT_PR_DATA
                this->num_expensive_queries++;
#endif
    PRINT_DEBUG("  Running 2-way BFS...")
    // start two-way BFS
    FindDiPathAlgorithm<FastPropertyMap> fpa;
    fpa.setGraph(this->diGraph);
    fpa.setConstructPaths(false, false);
    fpa.setSourceAndTarget(s, t);
    // fpa.prepare() omitted for performance reasons
    fpa.run();
    return fpa.deliver();
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
std::vector<Arc *>
SupportiveVerticesSloppySCCsAPRAlgorithm<
                DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>
    ::queryPath(Vertex *s, Vertex *t)
{
    auto emptyPath = std::vector<Arc*>();
    PRINT_DEBUG("Processing reachability query " << s << " -> " << t << "...")
    if (s == t) {
#ifdef COLLECT_PR_DATA
        this->num_trivial_queries++;
#endif
        PRINT_DEBUG("  Same vertices, trivially true, return empty path.")
        return emptyPath;
    }
    if (this->diGraph->isSink(s) || this->diGraph->isSource(t)) {
#ifdef COLLECT_PR_DATA
        this->num_trivial_queries++;
#endif
        PRINT_DEBUG("  Source is sink or target is source, trivially false, return empty path.")
        return emptyPath;
    }

    if (this->supportiveVertexToSSRAlgorithm[s].first) {
#ifdef COLLECT_PR_DATA
        this->num_only_ssr_queries++;
#endif
        PRINT_DEBUG("  Source is supportive vertex.")
        return this->supportiveVertexToSSRAlgorithm[s].first->queryPath(t);
    }

    if (this->supportiveVertexToSSRAlgorithm[t].second) {
#ifdef COLLECT_PR_DATA
        this->num_only_ssr_queries++;
#endif
        PRINT_DEBUG("  Sink is supportive vertex.")
        return this->supportiveVertexToSSRAlgorithm[t].second->queryPath(s);
    }

    PRINT_DEBUG("  Trying to use SCC information...")
    // use SCC info
    auto getRep = [this](Vertex *v) -> Vertex* {
        PRINT_DEBUG("  Looking up representative for " << v << "...")
        auto rep = this->vertexToSCCRepresentative(v);
        PRINT_DEBUG("    v2r map yields " << rep)
        if (!rep) {
            return nullptr;
        }
        const auto &[rSrc, rSink] = this->supportiveVertexToSSRAlgorithm(rep);
        // check!
        if (!rSrc || !rSink)  {
            // rep might have been deleted since last update
            this->vertexToSCCRepresentative[v] = nullptr;
            return nullptr;
        }

        PRINT_DEBUG("    Checking whether information is up-to-date...")
        if (rSrc->query(v) && rSink->query(v)) {
            PRINT_DEBUG("      OK.")
            return rep;
        }
        PRINT_DEBUG("      FAILED.")
        this->vertexToSCCRepresentative[v] = nullptr;
        return nullptr;
    };

    auto sRep = getRep(s);
    PRINT_DEBUG("  Source has representative " << sRep)
    if (sRep) {
        // either s ->* sRep ->* t  => TRUE
        // or sRep ->* s, but sRep -/>* t  => FALSE
#ifdef COLLECT_PR_DATA
        num_scc_via_srep_queries++;
#endif
        PRINT_DEBUG("  Answering query via source rep.")
        auto ssrc = this->supportiveVertexToSSRAlgorithm(sRep).first;
        if (ssrc->query(t)) {
            auto svPath = this->supportiveVertexToSSRAlgorithm(sRep).second->queryPath(s);
            auto vtPath = ssrc->queryPath(t);
            svPath.insert(svPath.end(), vtPath.begin(), vtPath.end());
            return svPath;
        }
        return emptyPath;
    }

    auto tRep = getRep(t);
    PRINT_DEBUG("  Target has representative " << tRep)
    if (tRep) {
        // either s ->* tRep ->* t  => TRUE
        // or t ->* tRep, but s -/>* tRep  => FALSE
#ifdef COLLECT_PR_DATA
        num_scc_via_trep_queries++;
#endif
        PRINT_DEBUG("  Answering query via target rep.")
        auto ssink = this->supportiveVertexToSSRAlgorithm(tRep).second;
        if (ssink->query(s)) {
            auto svPath = ssink->queryPath(s);
            auto vtPath = this->supportiveVertexToSSRAlgorithm(tRep).first->queryPath(t);
            svPath.insert(svPath.end(), vtPath.begin(), vtPath.end());
            return svPath;
        }
        return emptyPath;
    }

    // fall back to standard SV algorithm
    for (const auto &[ssrc, ssink] : this->supportiveSSRAlgorithms) {
        auto vt = ssrc->query(t);
        auto sv = ssink->query(s);
        if (sv) {
            if (vt) {
#ifdef COLLECT_PR_DATA
                this->num_only_support_queries_svt++;
#endif
                PRINT_DEBUG("  Reachability established via supportive vertex "
                            << ssrc->getSource() <<  ".")
                auto svPath = ssink->queryPath(s);
                auto vtPath = ssrc->queryPath(t);
                svPath.insert(svPath.end(), vtPath.begin(), vtPath.end());
                return svPath;
            }
        } else if (ssink->query(t)) {
            // no path from s to v, but from t to v
#ifdef COLLECT_PR_DATA
                this->num_only_support_queries_tv++;
#endif
                PRINT_DEBUG("  Non-reachability established via supportive vertex "
                            << ssrc->getSource() <<  ".")
                return emptyPath;
        }
        if (!vt && ssrc->query(s)) {
            // no path from v to t, but from v to s
#ifdef COLLECT_PR_DATA
                this->num_only_support_queries_vs++;
#endif
                PRINT_DEBUG("  Non-reachability established via supportive vertex "
                            << ssrc->getSource() <<  ".")
                return emptyPath;
        }
    }

#ifdef COLLECT_PR_DATA
                this->num_expensive_queries++;
#endif
    PRINT_DEBUG("  Running 2-way BFS...")
    // start two-way BFS
    FindDiPathAlgorithm<FastPropertyMap> fpa;
    fpa.setGraph(this->diGraph);
    fpa.setConstructPaths(false, true);
    fpa.setSourceAndTarget(s, t);
    // fpa.prepare() omitted for performance reasons
    fpa.run();
    if (fpa.deliver()) {
        return fpa.deliverArcsOnPath();
    }
    return emptyPath;
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
void
SupportiveVerticesSloppySCCsAPRAlgorithm<
                DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>
    ::onDiGraphSet()
{
    Super::onDiGraphSet();

    num_scc_via_srep_queries = 0;
    num_scc_via_trep_queries = 0;
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
void
SupportiveVerticesSloppySCCsAPRAlgorithm<
                DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>
    ::checkSCCs()
{
    PRINT_DEBUG("Checking SCCs...")
    TarjanSCCAlgorithm<FastPropertyMap> tarjan;
    tarjan.setGraph(this->diGraph);
    FastPropertyMap<DiGraph::size_type> sccs(this->diGraph->getSize() + 1);
    sccs.resetAll(this->diGraph->getSize());
    tarjan.useModifiableProperty(&sccs);
    if (!tarjan.prepare()) {
        throw DiGraphAlgorithmException(this, "Could not prepare TarjanSCC algorithm.");
    }
    tarjan.run();
    auto numSccs = tarjan.deliver();
    PRINT_DEBUG("  Found " << numSccs << " SCCs.")

    if (numSccs > 1) {
        std::vector<Vertex*> sccIdToRepresentative(numSccs, nullptr);
        std::vector<Vertex*> repsToClear;
        for (auto &p : this->supportiveSSRAlgorithms) {
            auto rep = p.first->getSource();
            PRINT_DEBUG("  Checking old SV " << rep << "...")
            auto sccId = sccs(rep);
            assert(sccId < numSccs);
            if (!sccIdToRepresentative[sccId]) {
                sccIdToRepresentative[sccId] = rep;
                PRINT_DEBUG("    Keeping it." )
            } else {
                repsToClear.push_back(rep);
                PRINT_DEBUG("    Will be removed." )
            }
        }
        for (auto *v : repsToClear) {
            this->removeSupportiveVertex(v);
        }
        //if (this->supportiveSSRAlgorithms.size() < numSccs) {
            std::vector<std::vector<Vertex*>> verticesInScc(numSccs);
            //std::vector<Vertex*> candidateReps;
            std::vector<DiGraph::size_type> uncoveredSccs;
            this->diGraph->mapVertices(
                        [&sccIdToRepresentative,&sccs,&verticesInScc,
                         //&candidateReps,
                         &uncoveredSccs,
                        this](Vertex *v) {
                auto sccId = sccs(v);
                assert(sccId < sccIdToRepresentative.size());
                PRINT_DEBUG("  Checking vertex " << v << " in SCC " << sccId << "...")
                if (sccIdToRepresentative[sccId]) {
                    vertexToSCCRepresentative[v] = sccIdToRepresentative[sccId];
                    PRINT_DEBUG("    Representative for " << v << " is "
                                << sccIdToRepresentative[sccId])
                } else {
                    vertexToSCCRepresentative[v] = nullptr;
                    if (verticesInScc[sccId].empty()) {
                        PRINT_DEBUG("    No old representative, but first of this SCC.")
                        verticesInScc[sccId].push_back(v);
                        uncoveredSccs.push_back(sccId);
                    } else {
                        PRINT_DEBUG("    No old representative.")
                        verticesInScc[sccId].push_back(v);
                    }
                }
            });
            PRINT_DEBUG("  Updating v2r map...")
            DiGraph::size_type emergencySccId = numSccs;
            for (auto sccId : uncoveredSccs) {
                auto sccSize = verticesInScc[sccId].size();
                if (sccSize >= this->supportSize) {
                    auto rep = verticesInScc[sccId].front();
                    PRINT_DEBUG("    " << rep << " becomes representative for SCC " << sccId)
                    createSupportVertex(rep);
                    for (auto *v : verticesInScc[sccId]) {
                        vertexToSCCRepresentative[v] = rep;
                    }
                } else {
                    PRINT_DEBUG("    SCC " << sccId << " has only size "
                                << verticesInScc[sccId].size()
                                << ", no representative selected.")
                    if (emergencySccId >= numSccs
                        || verticesInScc[emergencySccId].size() < sccSize) {
                        auto rep = verticesInScc[sccId].front();
                        if (!this->diGraph->isSource(rep) && !this->diGraph->isSink(rep)) {
                            emergencySccId = sccId;
                        }
                    }
                }
            }
            if (this->supportiveSSRAlgorithms.empty() && emergencySccId < numSccs) {
                auto rep = verticesInScc[emergencySccId].front();
                PRINT_DEBUG("    " << rep << " becomes representative for emergency SCC " << sccId)
                        createSupportVertex(rep);
                for (auto *v : verticesInScc[emergencySccId]) {
                    vertexToSCCRepresentative[v] = rep;
                }
            }
        //}
    } else {
        if (this->supportiveSSRAlgorithms.empty()) {
            createSupportVertex(this->diGraph->getAnyVertex());
        } else {
            while (this->supportiveSSRAlgorithms.size() > 1) {
                this->removeSupportiveVertex(
                            this->supportiveSSRAlgorithms.back().first->getSource());
            }
        }
        PRINT_DEBUG("  Just one SCC, setting all entries in v2r map to "
                    << this->supportiveSSRAlgorithms.back().first->getSource())
        // set representative vertex
        this->vertexToSCCRepresentative.setDefaultValue(
                    this->supportiveSSRAlgorithms.back().first->getSource());
        this->vertexToSCCRepresentative.resetAll(this->diGraph->getSize());
        this->vertexToSCCRepresentative.setDefaultValue(nullptr);
    }
}

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm, bool reAdjust>
void
SupportiveVerticesSloppySCCsAPRAlgorithm<
                DynamicSingleSourceAlgorithm, DynamicSingleSinkAlgorithm, reAdjust>
    ::createSupportVertex(Vertex *v)
{
    PRINT_DEBUG("  Creating SV " << v)
    this->createAndInitAlgorithm(v);
}

}
