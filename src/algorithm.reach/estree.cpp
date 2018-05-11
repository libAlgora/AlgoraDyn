#include "estree.h"

//#include <forward_list>
#include <vector>
#include <queue>
#include <climits>

#include "algorithm.basic/breadthfirstsearch.h"
#include "algorithm/digraphalgorithmexception.h"

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
auto cmp = [](ESTree::VertexData *l, ESTree::VertexData *r) { return (l->level  > r->level); };
typedef std::priority_queue<ESTree::VertexData*, std::vector<ESTree::VertexData*>, decltype(cmp)> PriorityQueue;

#ifdef DEBUG_ESTREE
void printQueue(PriorityQueue q) {
    std::cout << "PriorityQueue: ";
    while(!q.empty()) {
        std::cout << q.top()->vertex << "[" << q.top()->level << "]" << ", ";
        q.pop();
    }
    std::cout << std::endl;
}
#endif

void process(DiGraph *graph, ESTree::VertexData *vd, PriorityQueue &queue);

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
    PRINT_DEBUG("An arc is about to be removed: (" << a->getTail() << ", " << a->getHead() << ")")

   if (!initialized) {
        return;
    }

    Vertex *tail = a->getTail();
    Vertex *head = a->getHead();

    PriorityQueue queue(cmp);
    if (tail != root) {
        queue.push(data(tail));
        PRINT_DEBUG("Added " << tail << " to queue.")
    }
    if (head != root) {
        queue.push(data(head));
        PRINT_DEBUG("Added " << head << " to queue.")
    }

    while (!queue.empty()) {
        IF_DEBUG(printQueue(queue))
        auto vd = queue.top();
        queue.pop();
        process(diGraph, vd, queue);
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

void process(DiGraph *graph, ESTree::VertexData *vd, PriorityQueue &queue) {
    PRINT_DEBUG("Processing vertex " << vd->vertex)
    ESTree::VertexData *parent = vd->inNeighbors[vd->parentIndex];
    PRINT_DEBUG("  Current parent is " << parent->vertex << ".")
    bool parentChanged = false;
    while (vd->isReachable() && (graph->findArc(parent->vertex, vd->vertex) == nullptr || vd->level <= parent->level)) {
    //while (parent == nullptr || vd->level < parent->level + 1) {
        vd->parentIndex++;
        parentChanged = true;
        PRINT_DEBUG("  Advancing parent index.")

        if (vd->parentIndex >= vd->inNeighbors.size()) {
            PRINT_DEBUG("  Maximum parent index exceeded, increasing level.")
            vd->level++;
            if (vd->level >= graph->getSize()) {
                PRINT_DEBUG("    Vertex is unreachable.")
                vd->setUnreachable();
            }
            vd->parentIndex = 0;
            for (auto nd : vd->outNeighbors) {
                //if (nd != nullptr) {
                if (graph->findArc(vd->vertex, nd->vertex) != nullptr) {
                    queue.push(nd);
                    PRINT_DEBUG("    Added child " << nd->vertex << " to queue.")
                }
            }

        }
        if (vd->isReachable())  {
            parent = vd->inNeighbors[vd->parentIndex];
            PRINT_DEBUG("  Trying " << parent->vertex << " as parent.")
        }
    }
    if (parentChanged && vd->isReachable())  {
        queue.push(vd);
        PRINT_DEBUG("  Added " << vd->vertex << " to queue again.")
    }
}

}
