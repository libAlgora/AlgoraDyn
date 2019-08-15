/**
 * Copyright (C) 2013 - 2019 : Kathrin Hanauer
 *
 * This file is part of Algora.
 *
 * Algora is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Algora is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Algora.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact information:
 *   http://algora.xaikal.org
 */

#include "simpleincssreachalgorithm.h"

#include "algorithm.basic.traversal/breadthfirstsearch.h"
#include "algorithm/digraphalgorithmexception.h"
#include "property/fastpropertymap.h"
#include "graph/vertex.h"

#include <vector>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cassert>
#include <algorithm>

//#define DEBUG_SISSREACH

#ifdef DEBUG_SISSREACH
#include <iostream>
#define PRINT_DEBUG(msg) std::cout << msg << std::endl
#define IF_DEBUG(cmd) cmd;
#else
#define PRINT_DEBUG(msg)
//#define PRINT_DEBUG(msg) ((void)0)
#define IF_DEBUG(cmd)
#endif

namespace Algora {

struct SimpleIncSSReachAlgorithm::Reachability {
    enum struct State : std::int8_t { REACHABLE, UNREACHABLE, UNKNOWN };

    SimpleIncSSReachAlgorithm *parent;
    FastPropertyMap<State> reachability;
    FastPropertyMap<Arc*> pred;
    DiGraph *diGraph;
    Vertex *source;
    std::vector<const Vertex*> changedStateVertices;

    bool reverse;
    bool searchForward;
    double maxUnknownStateRatio;
    bool maxUSSqrt;
    bool maxUSLog;
    bool relateToReachable;
    bool radicalReset;

    DiGraph::size_type numReachable;
    profiling_counter numUnreached;
    profiling_counter numRereached;
    profiling_counter numUnknown;
    profiling_counter numReached;
    profiling_counter numTracebacks;
    DiGraph::size_type maxUnreached;
    DiGraph::size_type maxRereached;
    DiGraph::size_type maxUnknown;
    DiGraph::size_type maxReached;
    profiling_counter maxTracebacks;
    profiling_counter numReReachFromSource;
    profiling_counter incNonTreeArc;
    profiling_counter incUnReachableTail;
    profiling_counter decNonTreeArc;
    profiling_counter decUnReachableHead;

    Reachability(SimpleIncSSReachAlgorithm *p, bool r, bool sf, double maxUS)
        : parent(p), diGraph(nullptr), source(nullptr), reverse(r), searchForward(sf), maxUnknownStateRatio(maxUS),
          maxUSSqrt(false), maxUSLog(false), relateToReachable(false), numReachable(0U),
          numUnreached(0UL), numRereached(0UL), numUnknown(0UL), numReached(0UL), numTracebacks(0UL),
          maxUnreached(0UL), maxRereached(0UL), maxUnknown(0UL), maxReached(0UL), maxTracebacks(0UL),
          numReReachFromSource(0U),
          incNonTreeArc(0U), incUnReachableTail(0U), decNonTreeArc(0U), decUnReachableHead(0U) {
        reachability.setDefaultValue(State::UNREACHABLE);
        pred.setDefaultValue(nullptr);
        radicalReset = parent->radicalReset;
    }

    void reset(Vertex *src = nullptr) {
        source = src;
        reachability.resetAll();
        pred.resetAll();
        numReachable = 0UL;
#ifdef COLLECT_PR_DATA
        numUnreached = 0UL;
        numRereached = 0UL;
        numUnknown = 0UL;
        numReached = 0UL;
        numTracebacks = 0UL;
        maxUnreached = 0UL;
        maxRereached = 0UL;
        maxUnknown = 0UL;
        maxReached = 0UL;
        maxTracebacks = 0UL;
        numReReachFromSource = 0UL;
        incNonTreeArc = 0U;
        incUnReachableTail = 0U;
        decNonTreeArc = 0U;
        decUnReachableHead = 0U;
#endif
    }

    template<bool collectVertices, bool setPred, bool force, bool limit = false>
    DiGraph::size_type propagate(const Vertex *from, State s, DiGraph::size_type maxSteps = 0U) {
        PRINT_DEBUG("Propagating " << printState(s) << " from " << from << ".");
        DiGraph::size_type steps = 1U;
        BreadthFirstSearch<FastPropertyMap,false> bfs(false);
        bfs.setGraph(diGraph);
        bfs.setStartVertex(from);
        if (!setPred) {
            pred.resetToDefault(from);
        }
        if (reachability(from) != s) {
            reachability[from] = s;
            if (s == State::REACHABLE) {
                numReachable++;
            }
            if (collectVertices) {
                changedStateVertices.push_back(from);
            }
        }

        bfs.onArcDiscover([this,from,s,&steps](const Arc *ca) {
            auto *a = const_cast<Arc*>(ca);
            PRINT_DEBUG("Discovering arc (" << a->getTail() << ", " << a->getHead() << ")" );
#ifdef COLLECT_PR_DATA
            parent->prArcConsidered();
#endif
            auto v = a->getHead();

            PRINT_DEBUG("Reaching " << v << " via " << p << " with state " << printState(reachability(v))
                        << " and tree arc " << pred(v));

            if (pred(v) != nullptr && pred(v) != a) {
                PRINT_DEBUG(v << " already has another predecessor.");
                return false;
            }

            if ((!force && reachability(v) == s) || (v == source && source != from)) {
                PRINT_DEBUG(v << " already had this state and update was not forced, no update of successors.");
                return false;
            }

            if (!force && reachability(v) == State::UNREACHABLE && s == State::UNKNOWN) {
                PRINT_DEBUG(v << " is already known to be unreachable.");
                return false;
            }

            if (setPred && (pred(v) == nullptr || force)) {
                pred[v] = a;
                PRINT_DEBUG("Set predecessor.");
            } else if (!setPred && pred(v) == a) {
                pred.resetToDefault(v);
                PRINT_DEBUG("Deleted predecessor.");
            } else {
                PRINT_DEBUG("Warning: Predecessor not updated.");
                assert(false);
            }

            if (reachability[v] != s) {
                reachability[v] = s;
                if (s == State::REACHABLE) {
                    numReachable++;
                } else if (s == State::UNREACHABLE) {
                    assert(numReachable > 0U);
                    numReachable--;
                }
                PRINT_DEBUG(v << " gets new state.");
                if (collectVertices) {
                    changedStateVertices.push_back(v);
                }
            }
            if (limit > 0) {
                steps++;
            }
            return true;
        });
        if (limit) {
            bfs.setArcStopCondition([&steps,maxSteps](const Arc*) {
                return steps > maxSteps;
            });
            bfs.setVertexStopCondition([&steps,maxSteps](const Vertex*) {
                return steps > maxSteps;
            });
        }

#ifdef COLLECT_PR_DATA
        bfs.onVertexDiscover([this](const Vertex *) {
            parent->prVertexConsidered();
            return true;
        });
#endif
        if (!bfs.prepare()) {
            throw DiGraphAlgorithmException(nullptr, "Could not prepare BFS algorithm.");
        }
        bfs.run();
        return bfs.deliver();
    }

    bool checkReachability(const Vertex *u, std::vector<const Vertex*> &visitedUnknown) {
        assert (u != source);
        PRINT_DEBUG("Trying to find reachable predecessor of " << u << ".");
        BreadthFirstSearch<FastPropertyMap,false> bfs(false);
        bfs.setGraph(diGraph);
        bfs.reverseArcDirection(true);
        bfs.setStartVertex(u);

        assert(reachability(u) == State::UNKNOWN);
        visitedUnknown.push_back(u);

#ifdef COLLECT_PR_DATA
        bfs.onArcDiscover([this](const Arc *) {
            parent->prArcConsidered();
            return true;
        });
#endif

        FastPropertyMap<Arc*> succ;
        Vertex *reachableAncestor = nullptr;
        bfs.onTreeArcDiscover([this,&visitedUnknown,&succ,&reachableAncestor](const Arc* ca) {
            auto *a = const_cast<Arc*>(ca);

            auto v = a->getTail();

#ifdef COLLECT_PR_DATA
            parent->prVertexConsidered();
#endif

            PRINT_DEBUG("Exploring " << v->getName() << " with state " << printState(reachability(v))
                        << " via " << head);
            switch (reachability(v)) {
            case State::REACHABLE:
                reachableAncestor = v;
                succ[v] = a;
                return false;
            case State::UNKNOWN:
                visitedUnknown.push_back(v);
                succ[v] = a;
                return true;
            case State::UNREACHABLE:
                return false;
            }
            return true;
        });
        bfs.setArcStopCondition([&reachableAncestor](const Arc*) { return reachableAncestor != nullptr; });
        bfs.setVertexStopCondition([&reachableAncestor](const Vertex*) { return reachableAncestor != nullptr; });
        if (!bfs.prepare()) {
            throw DiGraphAlgorithmException(nullptr, "Could not prepare BFS algorithm.");
        }
        bfs.run();
        bfs.deliver();

        if (reachableAncestor != nullptr) {
            auto *t = reachableAncestor;
            auto *a = succ[t];
            auto *h = t;
            while (t != u) {
                a = succ[t];
                assert(a != nullptr);
                h = a->getHead();
#ifdef COLLECT_PR_DATA
                parent->prVertexConsidered();
#endif
                pred[h] = a;
                reachability[h] = State::REACHABLE;
                numReachable++;
                PRINT_DEBUG(t << " gets parent of " << h << ", " << h << " is now reachable.");
                t = h;
            }
            return true;
        }
        return false;
    }

    template<bool force = false>
    void reachFrom(const Vertex *from) {
#ifdef COLLECT_PR_DATA
        auto reached = propagate<false,true,force>(from, State::REACHABLE);
        if (reached > maxReached) {
            maxReached = reached;
        }
        numReached += reached;
#else
        propagate<false,true,force>(from, State::REACHABLE);
#endif
    }

    void unReachFrom(const Vertex *from) {
        if (from == source) {
            return;
        }

        if (!maxUSSqrt && !maxUSLog && maxUnknownStateRatio == 0.0) {
            PRINT_DEBUG("Maximum allowed unknown state ratio is 0, recomputing immediately.");
            reachability.resetAll();
            pred.resetAll();
            numReachable = 0U;
#ifdef COLLECT_PR_DATA
            numReReachFromSource++;
            parent->prReset();
#endif
            reachFrom<false>(source);
            return;
        }

        auto relateTo = relateToReachable ? numReachable : diGraph->getSize();
        auto compareTo = maxUSSqrt ? floor(sqrt(relateTo))
                                   : (maxUSLog ?
                                          floor(log2(relateTo)) : floor(maxUnknownStateRatio * relateTo));

        changedStateVertices.clear();
        auto maxSteps = static_cast<DiGraph::size_type>(compareTo);
#ifndef NDEBUG
        auto visited =
#endif
					radicalReset ? propagate<true, false, false, true>(from, State::UNKNOWN, maxSteps)
            : propagate<true, false, false, false>(from, State::UNKNOWN);

        auto unknown = changedStateVertices.size();
#ifdef COLLECT_PR_DATA
        numReachable -= unknown;
        PRINT_DEBUG( unknown << " vertices have unknown state, " << visited << " were visited by BFS.");
#endif
        assert (unknown == visited || unknown == visited + 1U);

        if (unknown > compareTo) {
            PRINT_DEBUG("Maximum allowed unknown state ratio exceeded, "
                        << unknown << " > " << compareTo << ", recomputing.");
#ifdef COLLECT_PR_DATA
            numReReachFromSource++;
            parent->prReset();
#endif

            if (radicalReset) {
                reachability.resetAll();
                pred.resetAll();
                numReachable = 0U;
                reachFrom<false>(source);
            } else {
                reachFrom<true>(source);
                for (auto v : changedStateVertices) {
#ifdef COLLECT_PR_DATA
                    parent->prVertexConsidered();
#endif
                    if (reachability(v) != State::REACHABLE) {
                        PRINT_DEBUG("Setting remaining vertex " << v << " with unknown state unreachable.");
                        reachability[v] = State::UNREACHABLE;
                    }
                }
            }
            changedStateVertices.clear();
            return;
        }

#ifdef COLLECT_PR_DATA
        auto rereached = 0UL;
        auto tracebacks = 0UL;
#endif
        std::vector<const Vertex*> backwardsReached;

        auto processUnknowns = [&](const Vertex *u) {
#ifdef COLLECT_PR_DATA
             parent->prVertexConsidered();
#endif
            if (reachability(u) == State::UNKNOWN) {
#ifdef COLLECT_PR_DATA
                tracebacks++;
#endif
                backwardsReached.clear();
                if (checkReachability(u, backwardsReached)) {
                    PRINT_DEBUG( u << " is reachable.");
                    if (searchForward) {
                        reachFrom(u);
                    }
                    assert(reachability[u] == State::REACHABLE);
                } else {
                    PRINT_DEBUG( u << " is unreachable.");
                    for (auto v : backwardsReached) {
                        PRINT_DEBUG("Setting backwards reached vertex " << v << " unreachable.");
                        assert(!reachable(v));
                        reachability[v] = State::UNREACHABLE;
                    }
                    assert(reachability[u] == State::UNREACHABLE);
                    backwardsReached.clear();
                }
            }
#ifdef COLLECT_PR_DATA
            if (reachability(u) == State::REACHABLE) {
                rereached++;
            }
#endif
        };

        if (reverse) {
            std::for_each(changedStateVertices.crbegin(), changedStateVertices.crend(), processUnknowns);
        } else {
            std::for_each(changedStateVertices.cbegin(), changedStateVertices.cend(), processUnknowns);
        }
        changedStateVertices.clear();


#ifdef COLLECT_PR_DATA
        assert(unknown >= rereached);
        auto unreached = unknown - rereached;
        numUnreached += unreached;
        numRereached += rereached;
        numUnknown += unknown;
        numTracebacks += tracebacks;
        if (maxUnreached < unreached) {
            maxUnreached = unreached;
        }
        if (maxRereached < rereached) {
            maxRereached = rereached;
        }
        if (maxUnknown < unknown) {
            maxUnknown = unknown;
        }
        if (maxTracebacks < tracebacks) {
            maxTracebacks = tracebacks;
        }
#endif
    }

    bool reachable(const Vertex *v) const {
        assert(reachability(source) == State::REACHABLE);
        return reachability(v) == State::REACHABLE;
    }

    void removeVertex(const Vertex *v) {
        // arcs must have already been removed
        assert(!reachable(v));
        reachability.resetToDefault(v);
    }

    char printState(const State &s) const {
        switch (s) {
        case SimpleIncSSReachAlgorithm::Reachability::State::REACHABLE:
            return 'R';
        case SimpleIncSSReachAlgorithm::Reachability::State::UNREACHABLE:
            return 'U';
        default:
            return '?';
        }
    }

    bool verifyReachability() const {
        FastPropertyMap<bool> lr(false);
        BreadthFirstSearch<FastPropertyMap,false> bfs(false);
        bfs.setStartVertex(source);
        bfs.onVertexDiscover([&lr](const Vertex *v) {
            lr[v] = true;
            return true;
        });
        runAlgorithm(bfs, diGraph);
        bool ok = true;
        auto countReachable = 0U;
        diGraph->mapVerticesUntil([&](Vertex *v) {
            if (reachability(v) == State::REACHABLE) {
                countReachable++;
            }

            if((reachability(v) == State::UNKNOWN)
                    || (reachability(v) == State::REACHABLE && !lr(v))
                    || (reachability(v) == State::UNREACHABLE && lr(v))) {
                ok = false;
                PRINT_DEBUG("State mismatch for vertex " << v << ": " << printState(reachability(v)) << " but is "
                            << (lr(v) ? "reachable" : "unreachable"));
            } else if (reachability(v) == State::REACHABLE && pred(v) == nullptr && v != source) {
                ok = false;
                PRINT_DEBUG(v << " is reachable but has no predecessor.");
            } else if (reachability(v) != State::REACHABLE && pred(v) != nullptr) {
                ok = false;
                PRINT_DEBUG(v << " is not reachable but has predecessor.");
            }
        }, [&](const Vertex *) { return !ok; });

        if (ok && countReachable != numReachable) {
            ok = false;
            PRINT_DEBUG("Wrong #reachable: stored " << numReachable << ", real " << countReachable );
        }
        return ok;
    }
};


SimpleIncSSReachAlgorithm::SimpleIncSSReachAlgorithm(bool reverse, bool searchForward, double maxUS, bool radicalReset)
    : DynamicSSReachAlgorithm(), initialized(false),
      reverse(reverse), searchForward(searchForward), maxUnknownStateRatio(maxUS),
      maxUSSqrt(false), maxUSLog(false), relateToReachable(false), radicalReset(radicalReset),
      data(new Reachability(this, reverse, searchForward, maxUS))

{
    registerEvents(false, true, true, true);
}

SimpleIncSSReachAlgorithm::~SimpleIncSSReachAlgorithm()
{
    delete data;
}

void SimpleIncSSReachAlgorithm::setMaxUnknownStateSqrt()
{
    maxUSSqrt = true;
    data->maxUSSqrt = true;
}

void SimpleIncSSReachAlgorithm::setMaxUnknownStateLog()
{
    maxUSLog = true;
    data->maxUSLog = true;
}

void SimpleIncSSReachAlgorithm::relateToReachableVertices(bool relReachable)
{
    relateToReachable = relReachable;
    data->relateToReachable = relReachable;
}

void SimpleIncSSReachAlgorithm::run()
{
    if (initialized) {
        return;
    }

    data->reset(source);
    data->reachFrom(source);
    initialized = true;
}

std::string SimpleIncSSReachAlgorithm::getName() const noexcept {
    std::stringstream ss;
    ss << "Simple Incremental Single-Source Reachability Algorithm ("
       << (reverse ? "reverse" : "non-reverse") << "/"
       << (searchForward ? "forward search" : "no forward search") << "/";
    if (maxUSSqrt) {
        ss << "SQRT";
    } else if (maxUSLog) {
        ss << "LOG";
    } else {
        ss << maxUnknownStateRatio;
    }
    ss << "*" << (relateToReachable ? "#R" : "#V") << "/";
    ss  << (radicalReset? "radical reset" : "soft reset") << ")";
    return ss.str();
}

std::string SimpleIncSSReachAlgorithm::getShortName() const noexcept {
    std::stringstream ss;
    ss << "Simple-ISSR("
       << (reverse ? "R" : "NR") << "/"
       << (searchForward ? "SF" : "NSF") << "/";
    if (maxUSSqrt) {
        ss << "SQRT";
    } else if (maxUSLog) {
        ss << "LOG";
    } else {
        ss << maxUnknownStateRatio;
    }
    ss << "~" << (relateToReachable ? "R" : "G") << "/";
    ss << (radicalReset ? "C" : "NC") << ")";
    return ss.str();
}

std::string SimpleIncSSReachAlgorithm::getProfilingInfo() const
{
    std::stringstream ss;
#ifdef COLLECT_PR_DATA
    ss << DynamicSSReachAlgorithm::getProfilingInfo();
    ss << "total reached vertices: " << data->numReached << std::endl;
    ss << "total unknown state vertices: " << data->numUnknown << std::endl;
    ss << "total unreached vertices: " << data->numUnreached << std::endl;
    ss << "total rereached vertices: " << data->numRereached << std::endl;
    ss << "total tracebacks: " << data->numTracebacks << std::endl;
    ss << "maximum reached vertices: " << data->maxReached << std::endl;
    ss << "maximum unreached vertices: " << data->maxUnreached << std::endl;
    ss << "maximum rereached vertices: " << data->maxRereached << std::endl;
    ss << "maximum unknown state vertices: " << data->maxUnknown << std::endl;
    ss << "maximum tracebacks: " << data->maxTracebacks << std::endl;
    ss << "unknown state limit: " << data->maxUnknownStateRatio << std::endl;
    ss << "#rereach from source: " << data->numReReachFromSource << std::endl;
    ss << "#unreachable head (dec): " << data->decUnReachableHead << std::endl;
    ss << "#non-tree arcs (dec): " << data->decNonTreeArc << std::endl;
    ss << "#unreachable tail (inc): " << data->incUnReachableTail << std::endl;
    ss << "#non-tree arcs (inc): " << data->incNonTreeArc << std::endl;
#endif
    return ss.str();
}

DynamicSSReachAlgorithm::Profile SimpleIncSSReachAlgorithm::getProfile() const
{
    auto profile = DynamicSSReachAlgorithm::getProfile();
    profile.push_back(std::make_pair(std::string("total_reached"), data->numReached));
    profile.push_back(std::make_pair(std::string("total_unknown"), data->numUnknown));
    profile.push_back(std::make_pair(std::string("total_unreached"), data->numUnreached));
    profile.push_back(std::make_pair(std::string("total_rereached"), data->numRereached));
    profile.push_back(std::make_pair(std::string("total_tracebacks"), data->numTracebacks));
    profile.push_back(std::make_pair(std::string("max_reached"), data->maxReached));
    profile.push_back(std::make_pair(std::string("max_unknown"), data->maxUnknown));
    profile.push_back(std::make_pair(std::string("max_unreached"), data->maxUnreached));
    profile.push_back(std::make_pair(std::string("max_rereached"), data->maxRereached));
    profile.push_back(std::make_pair(std::string("max_tracebacks"), data->maxTracebacks));
    profile.push_back(std::make_pair(std::string("unknown_limit_percent"), data->maxUnknownStateRatio * 100));
    profile.push_back(std::make_pair(std::string("rereach_from_source"), data->numReReachFromSource));
    profile.push_back(std::make_pair(std::string("dec_head_unreachable"), data->decUnReachableHead));
    profile.push_back(std::make_pair(std::string("dec_nontree"), data->decNonTreeArc));
    profile.push_back(std::make_pair(std::string("inc_tail_unreachable"), data->incUnReachableTail));
    profile.push_back(std::make_pair(std::string("inc_nontree"), data->incNonTreeArc));

    return profile;
}

void SimpleIncSSReachAlgorithm::onDiGraphSet()
{
    DynamicSSReachAlgorithm::onDiGraphSet();
    data->reset();
    data->diGraph = diGraph;
}

void SimpleIncSSReachAlgorithm::onDiGraphUnset() {
    initialized = false;
    DynamicSSReachAlgorithm::onDiGraphUnset();
}

void SimpleIncSSReachAlgorithm::onVertexAdd(Vertex *)
{
     // vertex is unreachable
}

void SimpleIncSSReachAlgorithm::onVertexRemove(Vertex *v)
{
    if (!initialized) {
        return;
    }
    data->removeVertex(v);
}

void SimpleIncSSReachAlgorithm::onArcAdd(Arc *a)
{
    if (!initialized) {
        return;
    }

    PRINT_DEBUG( "\nA new arc was added: (" << a->getTail() << ", " << a->getHead() << ")");

    if (a->isLoop()) {
        PRINT_DEBUG("Arc is a loop.");
        return;
    }
    Vertex *tail = a->getTail();
    Vertex *head = a->getHead();

    if (head == source) {
        PRINT_DEBUG("Head is source.");
        return;
    }

    if (!data->reachable(tail)) {
#ifdef COLLECT_PR_DATA
        data->incUnReachableTail++;
#endif
        return;
    }

    if (data->reachable(head)) {
#ifdef COLLECT_PR_DATA
        data->incNonTreeArc++;
#endif
        return;
    }

    data->pred[head] = a;
    data->reachFrom(head);
    assert(data->verifyReachability());
}

void SimpleIncSSReachAlgorithm::onArcRemove(Arc *a)
{
    if (!initialized) {
        return;
    }

    PRINT_DEBUG( "\n(" << a->getTail() << ", " << a->getHead() << ") is about to be removed" );

    if (a->isLoop()) {
        PRINT_DEBUG("Arc is a loop.");
        return;
    }
    Vertex *head = a->getHead();

    if (head == source) {
        PRINT_DEBUG("Head is source.");
        return;
    }

    if (!data->reachable(head)) {
        // head is already unreachable, nothing to update
        PRINT_DEBUG("Head is unreachable. Stop.");
#ifdef COLLECT_PR_DATA
        data->decUnReachableHead++;
#endif
        return;
    }

    if (data->pred(head) != a) {
        PRINT_DEBUG("Not a tree arc.");
#ifdef COLLECT_PR_DATA
        data->decNonTreeArc++;
#endif
        return;
    }

    data->unReachFrom(head);
    assert(data->verifyReachability());
}

bool SimpleIncSSReachAlgorithm::query(const Vertex *t)
{
    if (t == source) {
        return true;
    }

    if (!initialized) {
        run();
    }
    return data->reachable(t);
}

std::vector<Arc *> SimpleIncSSReachAlgorithm::queryPath(const Vertex *t)
{
    std::vector<Arc*> path;
    if (!query(t) || t == source) {
        return path;
    }

    while (t != source) {
        auto *a = data->pred(t);
        path.push_back(a);
        t = a->getTail();
    }
    assert(!path.empty());

    std::reverse(path.begin(), path.end());

    return path;
}

void SimpleIncSSReachAlgorithm::dumpData(std::ostream &os)
{
    if (!initialized) {
        os << "uninitialized" << std::endl;
    } else {
        os << "Source: " << source << std::endl;
        for (auto i = data->reachability.cbegin(); i != data->reachability.cend(); i++) {
            //os << (Vertex*) i->first << ": " << data->printState(i-> second) << std::endl;
            os << data->printState(*i) << std::endl;
        }
    }
}

void SimpleIncSSReachAlgorithm::onSourceSet()
{
    DynamicSSReachAlgorithm::onSourceSet();
    initialized = false;
    data->reset(source);
}

} // namespace
