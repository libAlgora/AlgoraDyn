#include "estree.h"

#include <vector>
#include <climits>

#include "algorithm.basic/breadthfirstsearch.h"
#include "algorithm/digraphalgorithmexception.h"
#include "datastructure/bucketqueue.h"

#define DEBUG_ESTREE

#ifdef DEBUG_ESTREE
#include <iostream>
#define PRINT_DEBUG(msg) std::cout << msg << std::endl;
#define IF_DEBUG(cmd) cmd;
#else
#define PRINT_DEBUG(msg)
#define IF_DEBUG(cmd)
#endif

namespace Algora {

struct ESTree::VertexData {
    Vertex *vertex;
    std::vector<VertexData*> outNeighbors;
    std::vector<VertexData*> inNeighbors;
    unsigned int parentIndex;
    unsigned int level;

    VertexData(Vertex *v, VertexData *p = nullptr, unsigned int l = UINT_MAX)
        : vertex(v), parentIndex(0), level(l) {
        if (p != nullptr) {
            inNeighbors.push_back(p);
            level = p->level + 1;
            p->outNeighbors.push_back(this);
        }
    }

    void setUnreachable() {
        parentIndex = 0;
        level = UINT_MAX;
    }

    bool isReachable() {
        return level != UINT_MAX;
    }
};

struct ESNode_Priority { int operator()(const ESTree::VertexData *vd) { return vd->level; }};
typedef BucketQueue<ESTree::VertexData*, ESNode_Priority> PriorityQueue;

#ifdef DEBUG_ESTREE
void printQueue(PriorityQueue q) {
    std::cout << "PriorityQueue: ";
    while(!q.empty()) {
        std::cout << q.bot()->vertex << "[" << q.bot()->level << "]" << ", ";
        q.popBot();
    }
    std::cout << std::endl;
}
#endif

void process(DiGraph *graph, ESTree::VertexData *vd, PriorityQueue &queue, const PropertyMap<ESTree::VertexData*> &data);

ESTree::ESTree()
    : DynamicSSReachAlgorithm(), root(nullptr), initialized(false)
{
   data.setDefaultValue(nullptr);
}

void ESTree::run()
{
    if (initialized) {
        return;
    }

    PRINT_DEBUG("Initializing ESTree...")

    data.resetAll();

   BreadthFirstSearch bfs(false);
   bfs.setGraph(this->diGraph);
   root = source;
   if (root == nullptr) {
       root =  diGraph->getAnyVertex();
   }
   bfs.setStartVertex(root);
   data[root] = new VertexData(root, nullptr, 0);
   bfs.onTreeArcDiscover([&](Arc *a) {
        Vertex *t = a->getTail();
        Vertex *h = a->getHead();
        data[h] = new VertexData(h, data(t));
        PRINT_DEBUG( "(" << t << ", " << h << ")" << " is a tree arc.")
   });
   bfs.onNonTreeArcDiscover([&](Arc *a) {
        VertexData *td = data(a->getTail());
        VertexData *hd = data(a->getHead());
        td->outNeighbors.push_back(hd);
        hd->inNeighbors.push_back(td);
        PRINT_DEBUG( "(" << td->vertex << ", " << hd->vertex << ")" << " is a non-tree arc.")
   });

   if (!bfs.prepare()) {
       throw DiGraphAlgorithmException(this, "Could not prepare BFS algorithm.");
   }
   bfs.run();
   bfs.deliver();

   initialized = true;
}

void ESTree::onDiGraphUnset()
{
    DynamicSSReachAlgorithm::onDiGraphUnset();
    initialized = false;
    data.resetAll();
}

void ESTree::onArcRemove(Arc *a)
{

   if (!initialized) {
        return;
    }

    PRINT_DEBUG("An arc is about to be removed: (" << a->getTail() << ", " << a->getHead() << ")")
    Vertex *head = a->getHead();
    if (head == root) {
        PRINT_DEBUG("Head of arc is root. Nothing to do.")
        return;
    }

    VertexData *hd = data(head);
    for (auto i = hd->inNeighbors.begin(); i != hd->inNeighbors.end(); i++) {
        if ((*i)->vertex == a->getTail()) {
           *i = nullptr;
           break; // multiple arcs?
        }
    }

    PriorityQueue queue;
    queue.push(hd);
    PRINT_DEBUG("Added " << head << " to queue.")

    while (!queue.empty()) {
        IF_DEBUG(printQueue(queue))
        auto vd = queue.bot();
        queue.popBot();
        process(diGraph, vd, queue, data);
    }
}

bool ESTree::query(const Vertex *t)
{
    if (!initialized) {
        run();
    }
    VertexData *d = data(t);
    return d != nullptr && d->isReachable();
}

void process(DiGraph *graph, ESTree::VertexData *vd, PriorityQueue &queue, const PropertyMap<ESTree::VertexData*> &data) {

    ESTree::VertexData *parent = vd->inNeighbors[vd->parentIndex];
    bool parentChanged = false;
    bool levelChanged = false;

    PRINT_DEBUG("Processing vertex " << vd->vertex << " on level " << vd->level << ".");
    PRINT_DEBUG("  Current parent is " << (parent == nullptr ? "null" : parent->vertex->toString())
                << " at index " << vd->parentIndex << ".");

    bool inNeighborFound = parent != nullptr;

    while (vd->isReachable() && (parent == nullptr || vd->level <= parent->level)) {
        vd->parentIndex++;
        parentChanged = true;
        PRINT_DEBUG("  Advancing parent index to " << vd->parentIndex << ".")

        if (vd->parentIndex >= vd->inNeighbors.size()) {
            if ((vd->level + 1 >= graph->getSize()) || (levelChanged && !inNeighborFound)) {
                PRINT_DEBUG("    Vertex is unreachable (source: " << (inNeighborFound ? "no" : "yes" ) << ").")
                vd->setUnreachable();
            } else {
                vd->level++;
                levelChanged = true;
                PRINT_DEBUG("  Maximum parent index exceeded, increasing level to " << vd->level << ".")
                vd->parentIndex = 0;
            }
        }
        if (vd->isReachable())  {
            parent = vd->inNeighbors[vd->parentIndex];
            inNeighborFound |= parent != nullptr;
            PRINT_DEBUG("  Trying " << (parent == nullptr ? "null" : parent->vertex->toString()) << " as parent.")
        }
    }
    if (parentChanged && vd->isReachable())  {
        queue.push(vd);
        PRINT_DEBUG("  Added " << vd->vertex << " to queue again.")
    }
    if (levelChanged) {
        graph->mapOutgoingArcs(vd->vertex, [&](Arc *a) {
            queue.push(data(a->getHead()));
            PRINT_DEBUG("    Added child " << a->getHead() << " to queue.")

        });
    }
}

}
