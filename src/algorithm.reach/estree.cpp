#include "estree.h"

#include <vector>
#include <climits>

#include "algorithm.basic/breadthfirstsearch.h"
#include "algorithm/digraphalgorithmexception.h"
#include "datastructure/bucketqueue.h"

//#define DEBUG_ESTREE

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
    std::vector<VertexData*> inNeighbors;
    unsigned int parentIndex;
    unsigned int level;

    VertexData(Vertex *v, VertexData *p = nullptr, unsigned int l = UINT_MAX)
        : vertex(v), parentIndex(0), level(l) {
        if (p != nullptr) {
            inNeighbors.push_back(p);
            level = p->level + 1;
        }
    }

    void setUnreachable() {
        parentIndex = 0;
        level = UINT_MAX;
        // free at least some space
        int i = inNeighbors.size() - 1;
        while (i >= 0 && inNeighbors[i] == nullptr) {
            inNeighbors.pop_back();
            i--;
        }
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
    knownArcs.setDefaultValue(false);
}

ESTree::~ESTree()
{
    if (diGraph == nullptr) {
        return;
    }

    for (auto i = data.cbegin(); i != data.cend(); i++) {
        delete i->second;
    }
    data.resetAll();
    knownArcs.resetAll();
}

void ESTree::run()
{
    if (initialized) {
        return;
    }

    PRINT_DEBUG("Initializing ESTree...")

    data.resetAll();
    knownArcs.resetAll();

   BreadthFirstSearch bfs(false);
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
        knownArcs[a] = true;
        PRINT_DEBUG( "(" << t << ", " << h << ")" << " is a tree arc.")
   });
   bfs.onNonTreeArcDiscover([&](Arc *a) {
        VertexData *td = data(a->getTail());
        VertexData *hd = data(a->getHead());
        hd->inNeighbors.push_back(td);
        knownArcs[a] = true;
        PRINT_DEBUG( "(" << td->vertex << ", " << hd->vertex << ")" << " is a non-tree arc.")
   });
   runAlgorithm(bfs, diGraph);

   initialized = true;
}

void ESTree::onDiGraphUnset()
{
    if (!initialized) {
        return;
    }
    diGraph->mapVertices([&](Vertex *v) {
        VertexData *vd = data(v);
        if (vd != nullptr) {
            delete vd;
        }
    });
    data.resetAll();
    knownArcs.resetAll();
    DynamicSSReachAlgorithm::onDiGraphUnset();
    initialized = false;
}

void ESTree::onArcAdd(Arc *a)
{
    if (!initialized) {
        return;
    }
    PRINT_DEBUG("An arc had been added: (" << a->getTail() << ", " << a->getHead() << ")")

    Vertex *tail = a->getTail();
    if (!query(tail)) {
        PRINT_DEBUG("Tail is unreachable.")
        return;
    }

    //update...
    VertexData *td = data(tail);
    Vertex *head = a->getHead();
    VertexData *hd = data(head);
    knownArcs[a] = true;
    if (hd == nullptr) {
       hd = new VertexData(head, td);
       data[head]  = hd;
    } else {
        hd->inNeighbors.push_back(td);
        if (hd->level <= td->level + 1) {
            // arc does not change anything
            return;
        } else {
            hd->level = td->level + 1;
        }
    }

    BreadthFirstSearch bfs(false);
    bfs.setStartVertex(head);
    bfs.onArcDiscover([&](const Arc *a) {
        PRINT_DEBUG( "Discovering arc (" << a->getTail() << ", " << a->getHead() << ")...");
        if (knownArcs(a)) {
            PRINT_DEBUG( "Arc is already known.");
            return false;
        }

        knownArcs[a] = true;
        Vertex *at = a->getTail();
        Vertex *ah = a->getHead();
        VertexData *atd = data(at);
        VertexData *ahd = data(ah);

        if (ahd == nullptr) {
            ahd = new VertexData(ah, atd);
            data[ah] = ahd;
            PRINT_DEBUG( "(" << at << ", " << ah << ")" << " is a new tree arc.");
            return true;
        }
        ahd->inNeighbors.push_back(atd);
        if (atd->level + 1 < ahd->level) {
            ahd->level = atd->level + 1;
            PRINT_DEBUG( "(" << at << ", " << ah << ")" << " replaces a tree arc.")
        } else {
            PRINT_DEBUG( "(" << at << ", " << ah << ")" << " is a non-tree arc.")
        }
        return false;
    });
    runAlgorithm(bfs, diGraph);

    restoreTree(hd);
}

void ESTree::onVertexRemove(Vertex *v)
{
    if (!initialized) {
        return;
    }

     VertexData *vd = data(v);
     if (vd != nullptr) {
         delete vd;
         data.resetToDefault(v);
     }
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
    restoreTree(hd);
    knownArcs.resetToDefault(a);
}

bool ESTree::query(const Vertex *t)
{
    if (!initialized) {
        run();
    }
    VertexData *d = data(t);
    return d != nullptr && d->isReachable();
}

void ESTree::restoreTree(ESTree::VertexData *rd)
{
    PriorityQueue queue;
    queue.push(rd);
    PRINT_DEBUG("Initialized queue with " << rd->vertex << ".")

    while (!queue.empty()) {
        IF_DEBUG(printQueue(queue))
        auto vd = queue.bot();
        queue.popBot();
        process(diGraph, vd, queue, data);
    }
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
                PRINT_DEBUG("    Vertex is unreachable (source: " << (levelChanged ? (inNeighborFound ? "no" : "yes" ) : "?") << ").")
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
