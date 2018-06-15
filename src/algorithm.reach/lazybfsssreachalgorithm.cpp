#include "lazybfsssreachalgorithm.h"

#include "property/propertymap.h"

#include <deque>

namespace Algora {

struct LazyBFSSSReachAlgorithm::CheshireCat {
    bool inititialized;
    DiGraph *graph;
    Vertex *source;
    std::deque<Vertex*> queue;
    PropertyMap<bool> discovered;

    CheshireCat()
        : inititialized(false) { discovered.setDefaultValue(false); }

    void searchOn(const Vertex *t) {
        if (!inititialized) {
            queue.clear();
            queue.push_back(source);
            discovered.resetAll();
            discovered[source] = true;
        }
        bool stop = false;
        while (!stop && !queue.empty()) {
            const Vertex *v = queue.front();
            queue.pop_front();
            graph->mapOutgoingArcs(v, [&](Arc *a) {
                Vertex *head = a->getHead();
                if (!discovered(head)) {
                    queue.push_back(head);
                    discovered[head] = true;
                    if (head == t) {
                        stop = true;
                    }
                }
            });
        }
        inititialized = true;
    }
};

LazyBFSSSReachAlgorithm::LazyBFSSSReachAlgorithm()
    : grin(new CheshireCat)
{

}

LazyBFSSSReachAlgorithm::~LazyBFSSSReachAlgorithm()
{
    delete grin;
}

void LazyBFSSSReachAlgorithm::run()
{

}

void LazyBFSSSReachAlgorithm::onDiGraphSet()
{
    DynamicSSReachAlgorithm::onDiGraphSet();
    grin->inititialized = false;
    grin->graph = diGraph;
}

void LazyBFSSSReachAlgorithm::onArcAdd(Arc *)
{
    grin->inititialized = false;
}

void LazyBFSSSReachAlgorithm::onArcRemove(Arc *)
{
    grin->inititialized = false;
}

bool LazyBFSSSReachAlgorithm::query(const Vertex *t)
{
    if (t == source || (grin->inititialized && grin->discovered(t))) {
        return true;
    }
    grin->searchOn(t);
    return grin->discovered(t);
}

void LazyBFSSSReachAlgorithm::onSourceSet()
{
    DynamicSSReachAlgorithm::onSourceSet();
    grin->inititialized = false;
    grin->source = source;
}

}
