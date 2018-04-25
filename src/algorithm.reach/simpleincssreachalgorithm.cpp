#include "simpleincssreachalgorithm.h"

#include "algorithm.basic/breadthfirstsearch.h"
#include "algorithm/digraphalgorithmexception.h"

#include <vector>

#define DEBUG_SISSREACH

#ifdef DEBUG_SISSREACH
#include <iostream>
#define PRINT_DEBUG(msg) std::cout << msg << std::endl;
#define IF_DEBUG(cmd) cmd;
#else
#define PRINT_DEBUG(msg)
#define IF_DEBUG(cmd)
#endif

namespace Algora {

SimpleIncSSReachAlgorithm::SimpleIncSSReachAlgorithm()
    : DynamicSSReachAlgorithm()
{
    isReachable.setDefaultValue(0);
}

void SimpleIncSSReachAlgorithm::run()
{
    if (initialized) {
        return;
    }

    isReachable.resetAll();

    reachFrom(source);
    initialized = true;
}

void SimpleIncSSReachAlgorithm::onDiGraphUnset() {
    DynamicSSReachAlgorithm::onDiGraphUnset();
    initialized = false;
    isReachable.resetAll();
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
    // arcs must have already been removed
    isReachable.resetToDefault(v);
}

void SimpleIncSSReachAlgorithm::onArcAdd(Arc *a)
{
    PRINT_DEBUG( "A new arc was added: (" << a->getTail() << ", " << a->getHead() << ")")
    if (!initialized) {
        run();
        return;
    }
    if (isReachable[a->getTail()] > 0) {
        reachFrom(a->getHead());
    }
}

void SimpleIncSSReachAlgorithm::onArcRemove(Arc *a)
{
    PRINT_DEBUG( "(" << a->getTail() << ", " << a->getHead() << ") is about to be removed" )
    if (!initialized) {
        PRINT_DEBUG("Uninitialized. Stop.")
        return;
    }

    if (isReachable(a->getTail()) == 0) {
        // tail is unreachable, nothing to update
        PRINT_DEBUG("Tail is unreachable. Stop.")
        return;
    }
    Vertex *head = a->getHead();
    PRINT_DEBUG("Head score " << isReachable(head))
    isReachable[head]--;
    if (isReachable(head) > 0) {
        PRINT_DEBUG("Head is still reachable. Stop.")
        // we're still good
        return;
    }

    std::vector<const Vertex*> unknown;
    BreadthFirstSearch bfs(false);
    bfs.setGraph(diGraph);
    bfs.setStartVertex(head);
    unknown.push_back(head);
    bfs.onVertexDiscover([&](const Vertex *v) {
        PRINT_DEBUG("Un-reaching " << v << " with score " << isReachable(v))
        isReachable[v]--;
        if (isReachable(v) > 0) {
            PRINT_DEBUG("Vertex is still reachable. Stop.")
            // successors need not be updated
            return false;
        }
        PRINT_DEBUG("Vertex state changed to unknown.")
        unknown.push_back(v);
        // continue
        return true;
    });
    if (!bfs.prepare()) {
        throw DiGraphAlgorithmException(this, "Could not prepare BFS algorithm.");
    }
    bfs.run();
    bfs.deliver();


    bfs.reverseArcDirection(true);
    while (!unknown.empty()) {
        const Vertex *u = unknown.back();
        unknown.pop_back();
        PRINT_DEBUG("Looking for alternative path to vertex " << u << ", score is " << isReachable(u))
        if (isReachable(u) > 0) {
                PRINT_DEBUG("Vertex is already reachable again.")
            continue;
        }
        bool reachable = false;
        bfs.setStartVertex(u);
        bfs.onVertexDiscover(vertexTrue);
        bfs.onArcDiscover([&](const Arc *da) {
            if (da == a) {
                PRINT_DEBUG("Arc is about to be removed. Can't use it.")
                return false;
            }
            Vertex *v = da->getTail(); // we're on reverse graph
            PRINT_DEBUG("Looking for alternative path via vertex " << v << ", score is " << isReachable(u))
            if (isReachable(v) > 0) {
                PRINT_DEBUG("Vertex is reachable, stopping search.")
                reachable = true;
                return false;
            }
            return true;
        });
        bfs.setVertexStopCondition([&](const Vertex *) { return reachable; });
        bfs.setArcStopCondition([&](const Arc*) { return reachable; });
        if (!bfs.prepare()) {
            throw DiGraphAlgorithmException(this, "Could not prepare BFS algorithm.");
        }
        bfs.run();
        if (reachable) {
            PRINT_DEBUG("Vertex " << u << " is reachable, score was " << isReachable(u))
            isReachable[u] =  1;
            reachFrom(u);
        } else {
            PRINT_DEBUG("Vertex " << u << " becomes unreachable, score is " << isReachable(u))
        }
    }
}

bool SimpleIncSSReachAlgorithm::query(const Vertex *t)
{
    if (!initialized) {
        run();
    }
    return isReachable[t] > 0;
}

void SimpleIncSSReachAlgorithm::reachFrom(const Vertex *s)
{
    PRINT_DEBUG(" Starting reachFrom " << s)
    isReachable[s]++;
    BreadthFirstSearch bfs(false);
    bfs.setGraph(diGraph);
    bfs.setStartVertex(s);
    bfs.onVertexDiscover([&](const Vertex *v) {
        PRINT_DEBUG("Reaching " << v << " with score " << isReachable(v) )
        isReachable[v]++;
        if (isReachable(v) > 1) {
             PRINT_DEBUG("no update of successors of " << v)
            // successors need not be updated
            return false;
        }
        // continue
        return true;
    });
    if (!bfs.prepare()) {
        throw DiGraphAlgorithmException(this, "Could not prepare BFS algorithm.");
    }
    bfs.run();
    bfs.deliver();
}

} // namespace
