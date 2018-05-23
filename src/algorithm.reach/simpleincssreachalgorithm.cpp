#include "simpleincssreachalgorithm.h"

#include "algorithm.basic/breadthfirstsearch.h"
#include "algorithm/digraphalgorithmexception.h"
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
    PropertyMap<State> reachability;
    Vertex *source;
    std::vector<const Vertex*> unknownStateVertices;

    Reachability() {
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

    void propagate(const Vertex *from, DiGraph *diGraph, State s) {
        PRINT_DEBUG("Propagating " << printState(s) << " from " << from << ".");
        reachability[from] = s;
        if (s == State::UNKNOWN) {
            unknownStateVertices.push_back(from);
        }
        BreadthFirstSearch bfs(false);
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
            if (s == State::UNKNOWN) {
                unknownStateVertices.push_back(v);
            }
            return true;
        });
        if (!bfs.prepare()) {
            throw DiGraphAlgorithmException(nullptr, "Could not prepare BFS algorithm.");
        }
        bfs.run();
        bfs.deliver();
    }

    bool checkReachability(const Vertex *u, DiGraph *diGraph) {
        assert (u != source);
        PRINT_DEBUG("Trying to find reachable predecessor of " << u << ".");
        bool reach = false;
        BreadthFirstSearch bfs(false);
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
        propagate(from, diGraph, State::REACHABLE);
    }

    void unReachFrom(const Vertex *from, DiGraph *diGraph) {
        if (from == source) {
            return;
        }
        unknownStateVertices.clear();
        propagate(from, diGraph, State::UNKNOWN);
        // TODO: does this help?
        std::reverse(unknownStateVertices.begin(), unknownStateVertices.end());
        while (!unknownStateVertices.empty()) {
           const Vertex *u = unknownStateVertices.back();
           unknownStateVertices.pop_back();
           if (reachability(u) == State::UNKNOWN) {
               // TODO: does this help?
               if (checkReachability(u, diGraph)) {
                    reachFrom(u, diGraph);
               }
           }
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


SimpleIncSSReachAlgorithm::SimpleIncSSReachAlgorithm()
    : DynamicSSReachAlgorithm(), data(new Reachability), initialized(false)
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
            os << (Vertex*) i->first << ": " << data->printState(i-> second) << std::endl;
        }
    }
}

} // namespace
