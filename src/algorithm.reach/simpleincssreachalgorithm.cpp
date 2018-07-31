/**
 * Copyright (C) 2013 - 2018 : Kathrin Hanauer
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
#include <cstdint>
#include <cassert>
#include <algorithm>

//#define DEBUG_SISSREACH

#ifdef DEBUG_SISSREACH
#include <iostream>
#define PRINT_DEBUG(msg) std::cout << msg << std::endl;
#define IF_DEBUG(cmd) cmd;
#else
#define PRINT_DEBUG(msg)
//#define PRINT_DEBUG(msg) ((void)0)
#define IF_DEBUG(cmd)
#endif

namespace Algora {

struct SimpleIncSSReachAlgorithm::Reachability {
    enum struct State : std::int8_t { REACHABLE, UNREACHABLE, UNKNOWN };
    FastPropertyMap<State> reachability;
    Vertex *source;
    std::vector<const Vertex*> changedStateVertices;

    bool reverse;
    bool searchForward;

    unsigned long numUnreached;
    unsigned long numRereached;
    unsigned long numUnknown;
    unsigned long numReached;
    unsigned long numTracebacks;
    unsigned long maxUnreached;
    unsigned long maxRereached;
    unsigned long maxUnknown;
    unsigned long maxReached;
    unsigned long maxTracebacks;

    Reachability(bool r, bool sf) : reverse(r), searchForward(sf),
      numUnreached(0UL), numRereached(0UL), numUnknown(0UL), numReached(0UL), numTracebacks(0UL),
      maxUnreached(0UL), maxRereached(0UL), maxUnknown(0UL), maxReached(0UL), maxTracebacks(0UL) {
        reachability.setDefaultValue(State::UNREACHABLE);
        source = nullptr;
    }

    void reset(Vertex *src = nullptr) {
        source = src;
        reachability.resetAll();
        if (source != nullptr) {
            reachability[source] = State::REACHABLE;
        }
    }

    unsigned int propagate(const Vertex *from, DiGraph *diGraph, State s, bool collectVertices) {
        PRINT_DEBUG("Propagating " << printState(s) << " from " << from << ".");
        reachability[from] = s;
        if (collectVertices) {
            changedStateVertices.push_back(from);
        }
        BreadthFirstSearch<FastPropertyMap> bfs(false);
        bfs.setGraph(diGraph);
        bfs.setStartVertex(from);
        bfs.onVertexDiscover([&](const Vertex *v) {
            PRINT_DEBUG("Reaching " << v << " with state " << printState(reachability(v)) );
            if (reachability(v) == s || v == source) {
                PRINT_DEBUG(v << " already had this state, no update of successors.");
                return false;
            }
            reachability[v] = s;
            PRINT_DEBUG(v << " gets new state.");
            if (collectVertices) {
                changedStateVertices.push_back(v);
            }
            return true;
        });
        if (!bfs.prepare()) {
            throw DiGraphAlgorithmException(nullptr, "Could not prepare BFS algorithm.");
        }
        bfs.run();
        return bfs.deliver();
    }

    bool checkReachability(const Vertex *u, DiGraph *diGraph) {
        assert (u != source);
        PRINT_DEBUG("Trying to find reachable predecessor of " << u << ".");
        bool reach = false;
        BreadthFirstSearch<FastPropertyMap> bfs(false);
        bfs.setGraph(diGraph);
        bfs.reverseArcDirection(true);
        bfs.setStartVertex(u);
        bfs.onVertexDiscover([&](const Vertex *v) {
            PRINT_DEBUG("Exploring " << v << " with state " << printState(reachability(v)) );
            if (reachable(v)) {
                reach = true;
                return false;
            }
            return true;
        });
        bfs.setArcStopCondition([&](const Arc*) { return reach; });
        bfs.setVertexStopCondition([&](const Vertex*) { return reach; });
        if (!bfs.prepare()) {
            throw DiGraphAlgorithmException(nullptr, "Could not prepare BFS algorithm.");
        }
        bfs.run();
        bfs.deliver();
        if (reach) {
            reachability[u] = State::REACHABLE;
            return true;
        }
        reachability[u] = State::UNREACHABLE;
        return false;
    }

    void reachFrom(const Vertex *from, DiGraph *diGraph) {
        auto reached = propagate(from, diGraph, State::REACHABLE, false);
        if (reached > maxReached) {
            maxReached = reached;
        }
        numReached += reached;
    }

    void unReachFrom(const Vertex *from, DiGraph *diGraph) {
        if (from == source) {
            return;
        }
        changedStateVertices.clear();
        propagate(from, diGraph, State::UNREACHABLE, true);

        if (reverse) {
            std::reverse(changedStateVertices.begin(), changedStateVertices.end());
        }
        auto unknown = changedStateVertices.size();
        auto rereached = 0UL;
        auto tracebacks = 0UL;
        while (!changedStateVertices.empty()) {
           const Vertex *u = changedStateVertices.back();
           changedStateVertices.pop_back();
           if (reachability(u) != State::REACHABLE) {
               tracebacks++;
               if (checkReachability(u, diGraph)) {
                   if (searchForward) {
                       reachFrom(u, diGraph);
                   } else {
                       reachability[u] = State::REACHABLE;
                   }
               }
           }
           if (reachability(u) == State::REACHABLE) {
               rereached++;
           }
        }

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
    }

    bool reachable(const Vertex *v) {
        return reachability(v) == State::REACHABLE;
    }

    void removeVertex(const Vertex *v) {
        // arcs must have already been removed
        assert(!reachable(v));
        reachability.resetToDefault(v);
    }

    char printState(const State &s){
        switch (s) {
        case SimpleIncSSReachAlgorithm::Reachability::State::REACHABLE:
            return 'R';
        case SimpleIncSSReachAlgorithm::Reachability::State::UNREACHABLE:
            return 'U';
            break;
        default:
            return '?';
        }
    }
};


SimpleIncSSReachAlgorithm::SimpleIncSSReachAlgorithm(bool reverse, bool searchForward)
    : DynamicSSReachAlgorithm(), data(new Reachability(reverse, searchForward)), initialized(false),
      reverse(reverse), searchForward(searchForward)

{ }

SimpleIncSSReachAlgorithm::~SimpleIncSSReachAlgorithm()
{
    delete data;
}

void SimpleIncSSReachAlgorithm::run()
{
    if (initialized) {
        return;
    }

    data->reset(source);
    data->reachFrom(source, diGraph);
    initialized = true;
}

std::string SimpleIncSSReachAlgorithm::getProfilingInfo() const
{
    std::stringstream ss;
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
    return ss.str();
}

DynamicSSReachAlgorithm::Profile SimpleIncSSReachAlgorithm::getProfile() const
{
    Profile profile;
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

    return profile;
}

void SimpleIncSSReachAlgorithm::onDiGraphUnset() {
    initialized = false;
    data->reset();
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
    Vertex *tail = a->getTail();
    Vertex *head = a->getHead();
    PRINT_DEBUG( "A new arc was added: (" << tail << ", " << head << ")")
    if (!initialized) {
        run();
        return;
    }
    if (data->reachable(tail)) {
        data->reachFrom(head, diGraph);
    }
}

void SimpleIncSSReachAlgorithm::onArcRemove(Arc *a)
{
    PRINT_DEBUG( "(" << a->getTail() << ", " << a->getHead() << ") is about to be removed" )
    if (!initialized) {
        PRINT_DEBUG("Uninitialized. Stop.")
        return;
    }

    if (!data->reachable(a->getTail())) {
        // tail is unreachable, nothing to update
        PRINT_DEBUG("Tail is unreachable. Stop.")
        return;
    }

    data->unReachFrom(a->getHead(), diGraph);
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
    initialized = false;
    data->reset();
}

} // namespace
