#include "estree.h"

//#include <forward_list>
#include <vector>
#include <queue>
#include <climits>

#include "algorithm.basic/breadthfirstsearch.h"
#include "algorithm/digraphalgorithmexception.h"

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

void process(DiGraph *graph, ESTree::VertexData *vd, PriorityQueue &queue);

ESTree::ESTree()
    : DynamicSSReachAlgorithm(), root(nullptr)
{

}

void ESTree::run()
{
   data.setDefaultValue(nullptr);
   BreadthFirstSearch bfs;
   bfs.setGraph(this->diGraph);
   if (root == nullptr) {
       root =  diGraph->getAnyVertex();
   }
   data[root] = new VertexData(root, nullptr, 0);
   bfs.onTreeArcDiscover([&](Arc *a) {
        Vertex *t = a->getTail();
        Vertex *h = a->getHead();
        data[h] = new VertexData(h, data(t));
   });
   bfs.onNonTreeArcDiscover([&](Arc *a) {
        VertexData *td = data(a->getTail());
        VertexData *hd = data(a->getHead());
        td->outNeighbors.push_back(hd);
        hd->inNeighbors.push_back(td);
   });

   if (!bfs.prepare()) {
       throw DiGraphAlgorithmException(this, "Could not prepare BFS algorithm.");
   }
   bfs.run();
   bfs.deliver();
}

void ESTree::onDiGraphUnset()
{
    DynamicSSReachAlgorithm::onDiGraphUnset();
    initialized = false;
    data.resetAll();
}

void ESTree::onArcRemove(Arc *a)
{
    Vertex *tail = a->getTail();
    Vertex *head = a->getHead();
    // todo "remove" arc

    PriorityQueue queue(cmp);
    queue.push(data(tail);
    queue.push(data(head);
    while (!queue.empty()) {
        auto vd = queue.top();
        queue.pop();
        process(diGraph, vd, queue);
    }
}

bool ESTree::query(const Vertex *t)
{
    VertexData *d = data(t);
    return d != nullptr;
}

void process(DiGraph *graph, ESTree::VertexData *vd, PriorityQueue &queue) {
    ESTree::VertexData *parent = vd->inNeighbors[vd->parentIndex];
    //while (graph->findArc(vd->vertex, parent->vertex) == nullptr || vd->level < parent->level + 1) {
    while (parent == nullptr || vd->level < parent->level + 1) {
        vd->parentIndex++;

        if (vd->parentIndex >= vd->inNeighbors.size()) {
            vd->level++;
            if (vd->level >= graph->getSize()) {
                vd->setUnreachable();
            } else {
                vd->parentIndex = 0;
                for (auto nd : vd->outNeighbors) {
                    if (nd != nullptr) {
                        queue.push(nd);
                    }
                }
            }
        }
        if (vd->isReachable())  {
            parent = vd->inNeighbors[vd->parentIndex];
        }
    }
    if (vd->isReachable())  {
        queue.push(vd);
    }
}

}
