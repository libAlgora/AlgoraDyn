#include "estree.h"

#include <vector>
#include <climits>
#include <cassert>

#include "graph/vertex.h"
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

std::ostream& operator<<(std::ostream& os, const ESTree::VertexData *vd) {
    if (vd == nullptr) {
        os << " null ";
        return os;
    }

    os << vd->vertex << ": ";
    os << "N-: [ ";
    for (auto nd : vd->inNeighbors) {
        if (nd == nullptr) {
            os << "null ";
        } else {
            os << nd->vertex << " ";
        }
    }
    os << "] ; parent: " << vd->parentIndex << " ; level: " << vd->level;
    return os;
}

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

ESTree::~ESTree()
{
    if (diGraph == nullptr) {
        return;
    }

    for (auto i = data.cbegin(); i != data.cend(); i++) {
        delete i->second;
    }
    data.resetAll();
}

void ESTree::run()
{
    if (initialized) {
        return;
    }

    PRINT_DEBUG("Initializing ESTree...")

    data.resetAll();

   BreadthFirstSearch bfs(false);
   root = source;
   if (root == nullptr) {
       root = diGraph->getAnyVertex();
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
        hd->inNeighbors.push_back(td);
        PRINT_DEBUG( "(" << td->vertex << ", " << hd->vertex << ")" << " is a non-tree arc.")
   });
   runAlgorithm(bfs, diGraph);

   diGraph->mapArcs([&](Arc *a) {
       Vertex *t = a->getTail();
       Vertex *h = a->getHead();
       VertexData *td = data(t);
       VertexData *hd = data(h);
       if (td == nullptr) {
           td = new VertexData(t);
           data[t] = td;
       }
       if (hd == nullptr) {
           hd = new VertexData(h);
           data[h] = hd;
       }
       if (!td->isReachable()) {
            hd->inNeighbors.push_back(td);
       }
   });

   initialized = true;
}

void ESTree::onDiGraphUnset()
{
    if (initialized) {
        diGraph->mapVertices([&](Vertex *v) {
            VertexData *vd = data(v);
            if (vd != nullptr) {
                delete vd;
            }
        });
        data.resetAll();
        initialized = false;
    }
    DynamicSSReachAlgorithm::onDiGraphUnset();
}

void ESTree::onVertexAdd(Vertex *v)
{
    data[v] = new VertexData(v);
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
        Vertex *at = a->getTail();
        Vertex *ah = a->getHead();
        VertexData *atd = data(at);
        VertexData *ahd = data(ah);

        if (atd->level + 1 < ahd->level) {
            ahd->level = atd->level + 1;
            PRINT_DEBUG( "(" << at << ", " << ah << ")" << " replaces a tree arc.");
            return true;
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

    Vertex *tail= a->getTail();
    Vertex *head = a->getHead();

    PRINT_DEBUG("An arc is about to be removed: (" << tail << ", " << head << ")");
    PRINT_DEBUG("Stored data of tail: " << data(tail));
    PRINT_DEBUG("Stored data of head: " << data(head));

    VertexData *hd = data(head);
    if (hd == nullptr) {
        PRINT_DEBUG("Head of arc is unreachable (and never was). Nothing to do.")
        return;
    }

    VertexData *td = data(tail);
    bool found = false;
    for (auto i = hd->inNeighbors.begin(); i != hd->inNeighbors.end() && !found; i++) {
        if ((*i) != nullptr && (*i) == td) {
           *i = nullptr;
           PRINT_DEBUG("Removed " << tail << " in N- of " << head);
           found = true;
        }
    }
    assert(found);

    if (hd->level <= data(tail)->level) {
        PRINT_DEBUG("Arc is not a tree arc. Nothing to do.")
        return;
    }

    if (!hd->isReachable()) {
        PRINT_DEBUG("Head of arc is already unreachable. Nothing to do.")
        return;
    }

    restoreTree(hd);
}

bool ESTree::query(const Vertex *t)
{
    if (t == source) {
        return true;
    }

    if (!initialized) {
        run();
    }
    VertexData *d = data(t);
    return d->isReachable();
}

void ESTree::dumpData(std::ostream &os)
{
    if (!initialized) {
        os << "uninitialized" << std::endl;
    }  else {
        for (auto i = data.cbegin(); i != data.cend(); i++) {
            os << i->first->toString() << ": " << i-> second << std::endl;
        }
    }
}

void ESTree::restoreTree(ESTree::VertexData *rd)
{
    PriorityQueue queue;
    queue.push(rd);
    PRINT_DEBUG("Initialized queue with " << rd << ".")

    while (!queue.empty()) {
        IF_DEBUG(printQueue(queue))
        auto vd = queue.bot();
        queue.popBot();
        process(diGraph, vd, queue, data);
    }
}

void process(DiGraph *graph, ESTree::VertexData *vd, PriorityQueue &queue, const PropertyMap<ESTree::VertexData*> &data) {

    if (vd->level == 0UL) {
        PRINT_DEBUG("No need to process source vertex " << vd << ".");
        return;
    }

    ESTree::VertexData *parent = vd->inNeighbors[vd->parentIndex];
    bool levelChanged = false;

    PRINT_DEBUG("Processing vertex " << vd << ".");

    bool inNeighborFound = parent != nullptr;

    while (vd->isReachable() && (parent == nullptr || vd->level <= parent->level)) {
        vd->parentIndex++;
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
            PRINT_DEBUG("  Trying " << parent << " as parent.")
        }
    }
    if (levelChanged) {
        graph->mapOutgoingArcs(vd->vertex, [&](Arc *a) {
            auto *hd = data(a->getHead());
            assert(hd->isReachable());
            queue.push(hd);
            PRINT_DEBUG("    Added child " << a->getHead() << " to queue.")
        });
    }
}

}
