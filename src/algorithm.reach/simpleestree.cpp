/**
 * Copyright (C) 2013 - 2018 : Kathrin Hanauer
 *
 * This file is part of Algora.
 *
 * Algora is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Algora is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Algora.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact information:
 *   http://algora.xaikal.org
 */

#include "simpleestree.h"

#include <vector>
#include <climits>
#include <cassert>

#include "graph/vertex.h"
#include "algorithm.basic.traversal/breadthfirstsearch.h"
#include "algorithm/digraphalgorithmexception.h"
#include "datastructure/bucketqueue.h"
#include "property/fastpropertymap.h"

//#define DEBUG_SIMPLEESTREE

#include <iostream>
#ifdef DEBUG_SIMPLEESTREE
#include <iostream>
#define PRINT_DEBUG(msg) std::cout << msg << std::endl;
#define IF_DEBUG(cmd) cmd;
#else
#define PRINT_DEBUG(msg)
#define IF_DEBUG(cmd)
#endif

namespace Algora {

struct SimpleESTree::VertexData {
    static const unsigned int UNREACHABLE = UINT_MAX;
    static unsigned int graphSize;

    Vertex *vertex;
    VertexData *parent;
    unsigned int level;

    VertexData(Vertex *v, VertexData *p = nullptr, unsigned int l = UINT_MAX)
        : vertex(v), parent(p), level(l) {
        if (p != nullptr) {
            level = p->level + 1;
        }
    }

    void setUnreachable() {
        parent = nullptr;
        level = UNREACHABLE;
    }

    bool isReachable() const {
        return level != UNREACHABLE;
    }

    unsigned int priority() const {
        return isReachable() ? level : graphSize + 1U;
    }

    bool isParent(VertexData *p) {
        return p == parent;
    }

    bool hasValidParent() const {
        return parent != nullptr && parent->level + 1 == level;
    }
};

unsigned int SimpleESTree::VertexData::graphSize = 0U;

std::ostream& operator<<(std::ostream& os, const SimpleESTree::VertexData *vd) {
    if (vd == nullptr) {
        os << " null ";
        return os;
    }

    os << vd->vertex << ": ";
    //os << "parent: [" << vd->parent << "] ; level: " << vd->level;
    os << "parent: [";
    if (vd->parent) {
      os << vd->parent->vertex << ", level: " << vd->parent->level;
    } else {
        os << "null";
    }
    os << "] ; level: " << vd->level;
    return os;
}

struct ESNode_Priority { int operator()(const SimpleESTree::VertexData *vd) { return vd->priority(); }};
typedef BucketQueue<SimpleESTree::VertexData*, ESNode_Priority> PriorityQueue;

#ifdef DEBUG_SIMPLEESTREE
void printQueue(PriorityQueue q) {
    std::cout << "PriorityQueue: ";
    while(!q.empty()) {
        std::cout << q.bot()->vertex << "[" << q.bot()->level << "]" << ", ";
        q.popBot();
    }
    std::cout << std::endl;
}
#endif

unsigned int process(DiGraph *graph, SimpleESTree::VertexData *vd, PriorityQueue &queue,
                     FastPropertyMap<SimpleESTree::VertexData*> &data, FastPropertyMap<bool> &reachable,
                     FastPropertyMap<bool> &inQueue);

SimpleESTree::SimpleESTree()
    : DynamicSSReachAlgorithm(), root(nullptr), initialized(false),
      movesDown(0U), movesUp(0U),
      levelIncrease(0U), levelDecrease(0U),
      maxLevelIncrease(0U), maxLevelDecrease(0U),
      decUnreachableHead(0U), decNonTreeArc(0U),
      incUnreachableTail(0U), incNonTreeArc(0U)
{
    data.setDefaultValue(nullptr);
    reachable.setDefaultValue(false);
}

SimpleESTree::~SimpleESTree()
{
    cleanup();
}

void SimpleESTree::run()
{
    if (initialized) {
        return;
    }

    PRINT_DEBUG("Initializing SimpleESTree...")

    data.resetAll(diGraph->getSize());
    reachable.resetAll(diGraph->getSize());
    movesDown = 0U;
    movesUp = 0U;

   BreadthFirstSearch<FastPropertyMap> bfs(false);
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
   runAlgorithm(bfs, diGraph);

   diGraph->mapVertices([&](Vertex *v) {
       if (data(v) == nullptr) {
           data[v] = new VertexData(v);
           PRINT_DEBUG( v << " is a unreachable.")
       }
   });

   VertexData::graphSize = diGraph->getSize();
   initialized = true;
    PRINT_DEBUG("Initializing completed.")
}

std::string SimpleESTree::getProfilingInfo() const
{
    std::stringstream ss;
    ss << "#moves down (level increase): " << movesDown << std::endl;
    ss << "#moves up (level decrease): " << movesUp << std::endl;
    ss << "total level increase: " << levelIncrease << std::endl;
    ss << "total level decrease: " << levelDecrease << std::endl;
    ss << "maximum level increase: " << maxLevelIncrease << std::endl;
    ss << "maximum level decrease: " << maxLevelDecrease << std::endl;
    ss << "#unreachable head (dec): " << decUnreachableHead << std::endl;
    ss << "#non-tree arcs (dec): " << decNonTreeArc << std::endl;
    ss << "#unreachable tail (inc): " << incUnreachableTail << std::endl;
    ss << "#non-tree arcs (inc): " << incNonTreeArc << std::endl;
    return ss.str();
}

DynamicSSReachAlgorithm::Profile SimpleESTree::getProfile() const
{
    Profile profile;
    profile.push_back(std::make_pair(std::string("vertices_moved_down"), movesDown));
    profile.push_back(std::make_pair(std::string("vertices_moved_up"), movesUp));
    profile.push_back(std::make_pair(std::string("total_level_increase"), levelIncrease));
    profile.push_back(std::make_pair(std::string("total_level_decrease"), levelDecrease));
    profile.push_back(std::make_pair(std::string("max_level_increase"), maxLevelIncrease));
    profile.push_back(std::make_pair(std::string("max_level_decrease"), maxLevelDecrease));
    profile.push_back(std::make_pair(std::string("dec_head_unreachable"), decUnreachableHead));
    profile.push_back(std::make_pair(std::string("dec_nontree"), decNonTreeArc));
    profile.push_back(std::make_pair(std::string("inc_tail_unreachable"), incUnreachableTail));
    profile.push_back(std::make_pair(std::string("inc_nontree"), incNonTreeArc));
    return profile;
}

void SimpleESTree::onDiGraphSet()
{
    DynamicSSReachAlgorithm::onDiGraphSet();
    cleanup();

    movesDown = 0U;
    movesUp = 0U;
    levelIncrease = 0U;
    levelDecrease = 0U;
    decUnreachableHead = 0U;
    decNonTreeArc = 0U;
    incUnreachableTail = 0U;
    incNonTreeArc = 0U;
}

void SimpleESTree::onDiGraphUnset()
{
    cleanup();
    DynamicSSReachAlgorithm::onDiGraphUnset();
}

void SimpleESTree::onVertexAdd(Vertex *v)
{
    data[v] = new VertexData(v);
    VertexData::graphSize++;
}

void SimpleESTree::onArcAdd(Arc *a)
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
        if (!hd->isReachable()) {
            levelDecrease += diGraph->getSize() -  (td->level + 1);
        } else {
            levelDecrease += hd->level - (td->level + 1);
        }
        hd->level = td->level + 1;
        reachable[head] = true;
        hd->parent = td;
    }

    auto n = diGraph->getSize();

    BreadthFirstSearch<FastPropertyMap> bfs(false);
    bfs.setStartVertex(head);
    bfs.onArcDiscover([&](const Arc *a) {
        PRINT_DEBUG( "Discovering arc (" << a->getTail() << ", " << a->getHead() << ")...");
        if (a->isLoop()) {
            PRINT_DEBUG( "Loop ignored.");
            return false;
        }
        Vertex *at = a->getTail();
        Vertex *ah = a->getHead();
        VertexData *atd = data(at);
        VertexData *ahd = data(ah);

        if (!ahd->isReachable() ||  atd->level + 1 < ahd->level) {
            movesUp++;
            auto newLevel = atd->level + 1;
            unsigned int dec;
            if (!ahd->isReachable()) {
                dec = n - newLevel;
            } else {
                dec = ahd->level - newLevel;
            }
            levelDecrease += dec;
            if (dec > maxLevelDecrease) {
                maxLevelDecrease = dec;
            }
            ahd->level = atd->level + 1;
            ahd->parent = atd;
            reachable[ah] = true;
            PRINT_DEBUG( "(" << at << ", " << ah << ")" << " replaces a tree arc.");
            return true;
        } else {
            PRINT_DEBUG( "(" << at << ", " << ah << ")" << " is a non-tree arc.")
        }
        return false;
    });
    runAlgorithm(bfs, diGraph);
}

void SimpleESTree::onVertexRemove(Vertex *v)
{
    VertexData::graphSize--;

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

void SimpleESTree::onArcRemove(Arc *a)
{
   if (!initialized) {
        return;
    }

    if (a->isLoop()) {
        PRINT_DEBUG("Arc is a loop.")
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
        throw std::logic_error("Should not happen");
        return;
    }

    if (!hd->isReachable()) {
        PRINT_DEBUG("Head of arc is already unreachable. Nothing to do.")
        decUnreachableHead++;
        return;
    }

    VertexData *td = data(tail);
    if (hd->isParent(td)) {
        hd->parent = nullptr;
        restoreTree(hd);
    } else {
        PRINT_DEBUG("Arc is not a tree arc. Nothing to do.")
        decNonTreeArc++;
    }
}

void SimpleESTree::onSourceSet()
{
    cleanup();
}

bool SimpleESTree::query(const Vertex *t)
{
    if (t == source) {
        return true;
    }

    if (!initialized) {
        std::cout << "Query in uninitialized state." << std::endl;
        run();
    }
    return reachable(t);
}

void SimpleESTree::dumpData(std::ostream &os)
{
    if (!initialized) {
        os << "uninitialized" << std::endl;
    }  else {
        for (auto i = data.cbegin(); i != data.cend(); i++) {
            os << (*i) << std::endl;
        }
    }
}

void SimpleESTree::restoreTree(SimpleESTree::VertexData *rd)
{
    PriorityQueue queue;
    FastPropertyMap<bool> inQueue(false);
    queue.push(rd);
    inQueue[rd->vertex] = true;
    PRINT_DEBUG("Initialized queue with " << rd << ".")

    while (!queue.empty()) {
        IF_DEBUG(printQueue(queue))
        auto vd = queue.bot();
        queue.popBot();
        inQueue.resetToDefault(vd->vertex);
        unsigned int levels = process(diGraph, vd, queue, data, reachable, inQueue);
        if (levels > 0) {
            movesDown++;
            levelIncrease += levels;
            if (levels > maxLevelIncrease) {
                maxLevelIncrease = levels;
            }
        }
    }
}

void SimpleESTree::cleanup()
{
    for (auto i = data.cbegin(); i != data.cend(); i++) {
        if ((*i)) {
            delete (*i);
        }
    }

    data.resetAll();
    reachable.resetAll();

    initialized = false;
}

unsigned int process(DiGraph *graph, SimpleESTree::VertexData *vd, PriorityQueue &queue,
                     FastPropertyMap<SimpleESTree::VertexData *> &data, FastPropertyMap<bool> &reachable,
                     FastPropertyMap<bool> &inQueue) {

    if (vd->level == 0UL) {
        PRINT_DEBUG("No need to process source vertex " << vd << ".");
        return 0U;
    }

    PRINT_DEBUG("Processing vertex " << vd << ".");
    Vertex *v = vd->vertex;

    auto oldParent = vd->parent;

    if (vd->hasValidParent()) {
        PRINT_DEBUG("Parent still valid. No further processing required.");
        return 0U;
    } else if (!vd->isReachable()) {
        PRINT_DEBUG("Vertex is already unreachable.");
        return 0U;
    }

    auto parent = oldParent;
    unsigned int oldVLevel = vd->level;
    unsigned int minParentLevel = parent == nullptr ? SimpleESTree::VertexData::UNREACHABLE : parent->level;

    PRINT_DEBUG("Min parent level is " << minParentLevel << ".");

    graph->mapIncomingArcsUntil(v, [&](Arc *a) {
        if (a->isLoop()) {
            PRINT_DEBUG( "Loop ignored.");
            return;
        }
        auto pd = data[a->getTail()];
        auto pLevel = pd->level;
        if (pLevel < minParentLevel) {
            minParentLevel = pLevel;
            parent = pd;
            PRINT_DEBUG("Update: Min parent level now is " << minParentLevel << ", parent " << parent);
            assert (minParentLevel + 1 >= oldVLevel);
        }
    }, [&oldVLevel, &minParentLevel](const Arc *) { return minParentLevel + 1 == oldVLevel; });

    unsigned int levelDiff = 0U;
    auto n = graph->getSize();

    if ((parent == nullptr || minParentLevel >= n - 1)
            && vd->isReachable()) {
        vd->setUnreachable();
        reachable.resetToDefault(v);
        levelDiff = n - oldVLevel;
        PRINT_DEBUG("No parent or parent is unreachable. Vertex is unreachable. Level diff " << levelDiff);
    } else if (parent != oldParent || oldVLevel <= minParentLevel) {
        assert(parent->isReachable());
        assert(minParentLevel != SimpleESTree::VertexData::UNREACHABLE);
        vd->level = minParentLevel + 1;
        vd->parent = parent;
        assert (vd->level >= oldVLevel);
        levelDiff = vd->level - oldVLevel;
        PRINT_DEBUG("Parent has changed, new parent is " << parent);
    }

    if (levelDiff > 0U) {
        PRINT_DEBUG("Updating children...");
        graph->mapOutgoingArcs(vd->vertex, [&](Arc *a) {
            if (a->isLoop()) {
                return;
            }
            Vertex *head = a->getHead();
            if (inQueue[head]) {
                PRINT_DEBUG("    Out-neighbor " << head << " already in queue.")
                return;
            }
            auto *hd = data(head);
            if (hd->isParent(vd)) {
                PRINT_DEBUG("    Adding child " << hd << " to queue...")
                queue.push(hd);
            }
        });
    }

    return levelDiff;
}

}
