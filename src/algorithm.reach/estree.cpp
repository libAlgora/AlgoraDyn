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

unsigned int process(DiGraph *graph, ESTree::VertexData *vd, PriorityQueue &queue, const PropertyMap<ESTree::VertexData*> &data, PropertyMap<bool> &reachable);

ESTree::ESTree()
    : DynamicSSReachAlgorithm(), root(nullptr), initialized(false),
      movesDown(0U), movesUp(0U)
{
    data.setDefaultValue(nullptr);
    reachable.setDefaultValue(false);
}

ESTree::~ESTree()
{
    cleanup();
}

void ESTree::run()
{
    if (initialized) {
        return;
    }

    PRINT_DEBUG("Initializing ESTree...")

    data.resetAll();
    reachable.resetAll();
    movesDown = 0U;
    movesUp = 0U;

   BreadthFirstSearch bfs(false);
   root = source;
   if (root == nullptr) {
       root = diGraph->getAnyVertex();
   }
   bfs.setStartVertex(root);
   data[root] = new VertexData(root, nullptr, 0);
   reachable[root] = true;
   bfs.onTreeArcDiscover([&](Arc *a) {
        Vertex *t = a->getTail();
        Vertex *h = a->getHead();
        data[h] = new VertexData(h, data(t));
        reachable[h] = true;
        PRINT_DEBUG( "(" << t << ", " << h << ")" << " is a tree arc.")
   });
   bfs.onNonTreeArcDiscover([&](Arc *a) {
        Vertex *t = a->getTail();
        Vertex *h = a->getHead();
        if (t == h) {
            return;
        }
        VertexData *td = data(t);
        VertexData *hd = data(h);
        hd->inNeighbors.push_back(td);
        PRINT_DEBUG( "(" << td->vertex << ", " << hd->vertex << ")" << " is a non-tree arc.")
   });
   runAlgorithm(bfs, diGraph);

   diGraph->mapArcs([&](Arc *a) {
       Vertex *t = a->getTail();
       Vertex *h = a->getHead();
       if (t == h) {
           return;
       }
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

   diGraph->mapVertices([&](Vertex *v) {
       if (data(v) == nullptr) {
           data[v] = new VertexData(v);
           PRINT_DEBUG( v << " is a unreachable.")
       }
   });

   initialized = true;
    PRINT_DEBUG("Initializing completed.")
}

std::string ESTree::getProfilingInfo() const
{
    std::stringstream ss;
    ss << "#moves down: " << movesDown << std::endl;
    ss << "#level increases: " << levelIncrease << std::endl;
    ss << "#moves up: " << movesUp << std::endl;
    ss << "#unreachable head (dec): " << decUnreachableHead << std::endl;
    ss << "#non-tree arcs (dec): " << decNonTreeArc << std::endl;
    ss << "#unreachable tail (inc): " << incUnreachableTail << std::endl;
    ss << "#non-tree arcs (inc): " << incNonTreeArc << std::endl;
    return ss.str();
}

void ESTree::onDiGraphUnset()
{
    cleanup();
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
    PRINT_DEBUG("An arc has been added: (" << a->getTail() << ", " << a->getHead() << ")")

    Vertex *tail = a->getTail();
    Vertex *head = a->getHead();
    if (tail == head) {
        PRINT_DEBUG("Arc is a loop.")
        return;
    }
    VertexData *td = data(tail);
    VertexData *hd = data(head);

    assert(td != nullptr);
    assert(hd != nullptr);

    // store arc
    hd->inNeighbors.push_back(td);

    if (!td->isReachable()) {
        PRINT_DEBUG("Tail is unreachable.")
        incUnreachableTail++;
        return;
    }

    //update...
    if (hd->level <= td->level + 1) {
        // arc does not change anything
        incNonTreeArc++;
        return;
    } else {
        movesUp++;
        hd->level = td->level + 1;
        movesUp++;
        reachable[head] = true;
    }

    std::vector<VertexData*> verticesToProcess;
    verticesToProcess.push_back(hd);

    BreadthFirstSearch bfs(false);
    bfs.setStartVertex(head);
    bfs.onArcDiscover([&](const Arc *a) {
        PRINT_DEBUG( "Discovering arc (" << a->getTail() << ", " << a->getHead() << ")...");
        Vertex *at = a->getTail();
        Vertex *ah = a->getHead();
        VertexData *atd = data(at);
        VertexData *ahd = data(ah);

        if (!ahd->isReachable() ||  atd->level + 1 < ahd->level) {
            ahd->level = atd->level + 1;
            movesUp++;
            reachable[ah] = true;
            verticesToProcess.push_back(ahd);
            PRINT_DEBUG( "(" << at << ", " << ah << ")" << " replaces a tree arc.");
            return true;
        } else {
            PRINT_DEBUG( "(" << at << ", " << ah << ")" << " is a non-tree arc.")
        }
        return false;
    });
    runAlgorithm(bfs, diGraph);

    //restoreTree(hd);
    for (auto vd : verticesToProcess) {
        restoreTree(vd);
    }
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
         reachable.resetToDefault(v);
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
    if (tail == head) {
        PRINT_DEBUG("Arc is a loop.")
        return;
    }

    PRINT_DEBUG("Stored data of tail: " << data(tail));
    PRINT_DEBUG("Stored data of head: " << data(head));

    VertexData *hd = data(head);
    if (hd == nullptr) {
        PRINT_DEBUG("Head of arc is unreachable (and never was). Nothing to do.")
        throw std::logic_error("Should not happen");
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

    if (!hd->isReachable()) {
    //if (!reachable(head)) {
        PRINT_DEBUG("Head of arc is already unreachable. Nothing to do.")
        decUnreachableHead++;
        return;
    }

    // todo: we can do more here
    if (hd->level <= td->level) {
        PRINT_DEBUG("Arc is not a tree arc. Nothing to do.")
        decNonTreeArc++;
        return;
    }

    restoreTree(hd);
}

void ESTree::onSourceSet()
{
    cleanup();
}

bool ESTree::query(const Vertex *t)
{
    if (t == source) {
        return true;
    }

    if (!initialized) {
        std::cout << "query in unitialized state." << std::endl;
        run();
    }
    return reachable(t);
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
        unsigned int levels = process(diGraph, vd, queue, data, reachable);
        if (levels > 0) {
            movesDown++;
            levelIncrease += levels;
        }
    }
}

void ESTree::cleanup()
{
    for (auto i = data.cbegin(); i != data.cend(); i++) {
        delete i->second;
    }

    data.resetAll();
    reachable.resetAll();

    initialized = false;

    //movesDown = 0U;
    //movesUp = 0U;
    //decUnreachableHead = 0U;
    //decNonTreeArc = 0U;
    //incUnreachableTail = 0U;
    //incNonTreeArc = 0U;
}

unsigned int process(DiGraph *graph, ESTree::VertexData *vd, PriorityQueue &queue, const PropertyMap<ESTree::VertexData*> &data, PropertyMap<bool> &reachable) {

    if (vd->level == 0UL) {
        PRINT_DEBUG("No need to process source vertex " << vd << ".");
        return 0U;
    }

    ESTree::VertexData *parent = vd->inNeighbors[vd->parentIndex];
    bool levelChanged = false;

    PRINT_DEBUG("Processing vertex " << vd << ".");
    PRINT_DEBUG("Size of graph is " << graph->getSize() << ".");

    bool inNeighborFound = parent != nullptr;
    bool reachableInNeighborFound = inNeighborFound && parent->isReachable();
    Vertex *v = vd->vertex;
    bool reachV = vd->isReachable();
    unsigned int minimumParentLevel = parent != nullptr ? parent->level : UINT_MAX;

    PRINT_DEBUG("Parent is " << parent);

    unsigned int levelDiff = 0U;

    while (reachV && (parent == nullptr || vd->level <= parent->level)) {
        vd->parentIndex++;
        PRINT_DEBUG("  Advancing parent index to " << vd->parentIndex << ".")

        if (vd->parentIndex >= vd->inNeighbors.size()) {
            if ((vd->level + 1 >= graph->getSize()) || (levelChanged && !inNeighborFound)) {
                PRINT_DEBUG("    Vertex is unreachable (source: " << (levelChanged ? (inNeighborFound ? "no" : "yes" ) : "?") << ").")
                assert(!reachableInNeighborFound);
                vd->setUnreachable();
                reachable.resetToDefault(v);
                reachV = false;
                levelChanged = true;
            } else {
                vd->level++;
                levelDiff++;
                levelChanged = true;
                PRINT_DEBUG("  Maximum parent index exceeded, increasing level to " << vd->level << ".")
                vd->parentIndex = 0;
            }
        }
        if (reachV)  {
            parent = vd->inNeighbors[vd->parentIndex];
            inNeighborFound |= parent != nullptr;
            reachableInNeighborFound |= (parent && parent->isReachable());
            if (parent && parent->level < minimumParentLevel) {
                minimumParentLevel = parent->level;
                PRINT_DEBUG("  Minimum parent level is now " << minimumParentLevel << ".")
            }
            PRINT_DEBUG("  Trying " << parent << " as parent.")
        }
    }
    if (levelChanged) {
        graph->mapOutgoingArcs(vd->vertex, [&](Arc *a) {
            Vertex *head = a->getHead();
            auto *hd = data(head);
            assert(hd->isReachable());
            assert(reachable(head));
            queue.push(hd);
            PRINT_DEBUG("    Added child " << a->getHead() << " to queue.")
        });
    }

    return levelDiff;
}

}
