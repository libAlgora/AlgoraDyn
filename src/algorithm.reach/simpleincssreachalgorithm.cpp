#include "simpleincssreachalgorithm.h"

#include "algorithm.basic/breadthfirstsearch.h"
#include "algorithm/digraphalgorithmexception.h"

#include <vector>

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
    if (!initialized) {
        return;
    }

    if (isReachable(a->getTail()) == 0) {
        // tail is unreachable, nothing to update
        return;
    }
    Vertex *head = a->getHead();
    isReachable[head]--;
    if (isReachable(head) > 0) {
        // we're still good
        return;
    }

    std::vector<Vertex*> unknown;
    BreadthFirstSearch bfs(false);
    bfs.setGraph(diGraph);
    bfs.setStartVertex(head);
    bfs.onVertexDiscover([&](Vertex *v) {
        isReachable[v]--;
        if (isReachable(v) > 0) {
            // successors need not be updated
            return false;
        }
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
        Vertex *u = unknown.back();
        unknown.pop_back();
        if (isReachable(u) > 0) {
            continue;
        }
        bool reachable = false;
        bfs.setStartVertex(u);
        bfs.onVertexDiscover([&](Vertex *v) {
            if (isReachable(v) > 0) {
                reachable = true;
                return false;
            }
            return true;
        });
        bfs.setVertexStopCondition([&](Vertex *) { return reachable; });
        bfs.setArcStopCondition([&](Arc*) { return reachable; });
        if (!bfs.prepare()) {
            throw DiGraphAlgorithmException(this, "Could not prepare BFS algorithm.");
        }
        bfs.run();
        if (reachable) {
            reachFrom(u);
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
    isReachable[s]++;
    BreadthFirstSearch bfs(false);
    bfs.setGraph(diGraph);
    bfs.setStartVertex(s);
    bfs.onVertexDiscover([&](const Vertex *v) {
        isReachable[v]++;
        if (isReachable(v) > 1) {
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
