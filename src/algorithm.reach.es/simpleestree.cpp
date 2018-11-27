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
#define PRINT_DEBUG(msg) std::cerr << msg << std::endl;
#define IF_DEBUG(cmd) cmd;
#else
#define PRINT_DEBUG(msg) ((void)0);
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

    void reset(VertexData *p = nullptr, unsigned int l = UINT_MAX) {
        parent = p;
        level = l;
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

    Vertex *getParent() const {
        if (parent == nullptr) {
            return nullptr;
        }
        return parent->vertex;
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
    std::cerr << "PriorityQueue: ";
    while(!q.empty()) {
        std::cerr << q.bot()->vertex << "[" << q.bot()->level << "]" << ", ";
        q.popBot();
    }
    std::cerr << std::endl;
}
#endif

unsigned int process(DiGraph *graph, SimpleESTree::VertexData *vd, PriorityQueue &queue,
                     const FastPropertyMap<SimpleESTree::VertexData*> &data,
                     FastPropertyMap<bool> &reachable,
                     FastPropertyMap<bool> &inQueue,
                     FastPropertyMap<unsigned int> &timesInQueue, unsigned int limit,
										 bool &limitReached, unsigned int &maxRequeued,
										 unsigned int &verticesConsidered, unsigned int &arcsConsidered);

SimpleESTree::SimpleESTree(unsigned int requeueLimit, double maxAffectedRatio)
    : DynamicSSReachAlgorithm(), root(nullptr),
      initialized(false), requeueLimit(requeueLimit),
      maxAffectedRatio(maxAffectedRatio), movesDown(0U), movesUp(0U),
      levelIncrease(0U), levelDecrease(0U),
      maxLevelIncrease(0U), maxLevelDecrease(0U),
      decUnreachableHead(0U), decNonTreeArc(0U),
      incUnreachableTail(0U), incNonTreeArc(0U),
      reruns(0U), maxReQueued(0U),
      maxAffected(0U), totalAffected(0U)
{
    data.setDefaultValue(nullptr);
    reachable.setDefaultValue(false);
}

SimpleESTree::~SimpleESTree()
{
    cleanup();
}

unsigned int SimpleESTree::getDepthOfBFSTree() const
{
    auto maxLevel = 0U;
    diGraph->mapVertices([&](Vertex *v) {
        if (reachable(v) && data(v)->level > maxLevel) {
            maxLevel = data(v)->level;
        }
    });
    return maxLevel + 1U;
}

unsigned int SimpleESTree::getNumReachable() const
{
    auto numR = 0U;
    diGraph->mapVertices([&](Vertex *v) {
        if (reachable(v)) {
            numR++;
        }
    });
    return numR;
}

void SimpleESTree::run()
{
    if (initialized) {
        return;
    }

    PRINT_DEBUG("Initializing SimpleESTree...")

    reachable.resetAll(diGraph->getSize());

   BreadthFirstSearch<FastPropertyMap> bfs(false);
   root = source;
   if (root == nullptr) {
       root = diGraph->getAnyVertex();
   }
   bfs.setStartVertex(root);
   if (data[root] == nullptr) {
      data[root] = new VertexData(root, nullptr, 0);
   } else {
       data[root]->reset(nullptr, 0);
   }
   reachable[root] = true;
   bfs.onTreeArcDiscover([&](Arc *a) {
#ifdef COLLECT_PR_DATA
        prVertexConsidered();
        prArcConsidered();
#endif
        Vertex *t = a->getTail();
        Vertex *h = a->getHead();
        if (data[h] == nullptr) {
            data[h] = new VertexData(h, data(t));
        } else {
            data[h]->reset(data(t));
        }
        reachable[h] = true;
        PRINT_DEBUG( "(" << t << ", " << h << ")" << " is a tree arc.")
#ifdef COLLECT_PR_DATA
        prArcConsidered();
#endif
   });
   runAlgorithm(bfs, diGraph);

   diGraph->mapVertices([&](Vertex *v) {
#ifdef COLLECT_PR_DATA
        prVertexConsidered();
#endif
       if (data(v) == nullptr) {
           data[v] = new VertexData(v);
           PRINT_DEBUG( v << " is a unreachable.")
       }
   });

   VertexData::graphSize = diGraph->getSize();
   initialized = true;
   PRINT_DEBUG("Initializing completed.");

   IF_DEBUG(
    if (!checkTree()) {
        std::cerr.flush();
        dumpTree(std::cerr);
        std::cerr.flush();
   });
   assert(checkTree());
}

std::string SimpleESTree::getProfilingInfo() const
{
    std::stringstream ss;
    ss << DynamicSSReachAlgorithm::getProfilingInfo();
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
    ss << "requeue limit: " << requeueLimit << std::endl;
    ss << "maximum #requeuings: " << maxReQueued << std::endl;
    ss << "maximum ratio of affected vertices: " << maxAffectedRatio << std::endl;
    ss << "total affected vertices: " << totalAffected << std::endl;
    ss << "maximum number of affected vertices: " << maxAffected << std::endl;
    ss << "#reruns: " << reruns << std::endl;
    return ss.str();
}

DynamicSSReachAlgorithm::Profile SimpleESTree::getProfile() const
{
    auto profile = DynamicSSReachAlgorithm::getProfile();
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
    profile.push_back(std::make_pair(std::string("requeue_limit"), requeueLimit));
    profile.push_back(std::make_pair(std::string("max_affected"), maxAffectedRatio));
    profile.push_back(std::make_pair(std::string("max_requeued"), maxReQueued));
    profile.push_back(std::make_pair(std::string("total_affected"), totalAffected));
    profile.push_back(std::make_pair(std::string("max_affected"), maxAffected));
    profile.push_back(std::make_pair(std::string("rerun"), reruns));
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
    reruns = 0U;
    maxReQueued = 0U;
    maxAffected = 0U;
    totalAffected = 0U;
    data.resetAll(diGraph->getSize());
    reachable.resetAll(diGraph->getSize());
    VertexData::graphSize = diGraph->getSize();
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

    if (a->isLoop()) {
        PRINT_DEBUG("Arc is a loop.")
        return;
    }

    Vertex *tail = a->getTail();
    Vertex *head = a->getHead();

    if (head == source) {
        PRINT_DEBUG("Head is source.")
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

    auto n = diGraph->getSize();

    //update...
    if (hd->level <= td->level + 1) {
        // arc does not change anything
        incNonTreeArc++;
        PRINT_DEBUG("Not a tree arc.")
        return;
    } else {
        PRINT_DEBUG("Is a new tree arc.")
        movesUp++;
        if (!hd->isReachable()) {
            levelDecrease += (n -  (td->level + 1));
        } else {
            levelDecrease += (hd->level - (td->level + 1));
        }
        hd->level = td->level + 1;
        reachable[head] = true;
        hd->parent = td;
    }

    BreadthFirstSearch<FastPropertyMap> bfs(false);
    bfs.setStartVertex(head);
    bfs.onArcDiscover([&](const Arc *a) {
        PRINT_DEBUG( "Discovering arc (" << a->getTail() << ", " << a->getHead() << ")...");
#ifdef COLLECT_PR_DATA
        prArcConsidered();
#endif
        if (a->isLoop()) {
            PRINT_DEBUG( "Loop ignored.");
            return false;
        }
        Vertex *at = a->getTail();
        Vertex *ah = a->getHead();
        VertexData *atd = data(at);
        VertexData *ahd = data(ah);

#ifdef COLLECT_PR_DATA
        prVertexConsidered();
#endif
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
            PRINT_DEBUG( "(" << at << ", " << ah << ")" << " is a new tree arc.");
            return true;
        } else {
            PRINT_DEBUG( "(" << at << ", " << ah << ")" << " is a non-tree arc.")
        }
        return false;
    });
    runAlgorithm(bfs, diGraph);

   IF_DEBUG(
    if (!checkTree()) {
        std::cerr.flush();
        dumpTree(std::cerr);
        std::cerr.flush();
   });
   assert(checkTree());
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

    PRINT_DEBUG("An arc is about to be removed: (" << a->getTail() << ", " << a->getHead() << ")");

    if (a->isLoop()) {
        PRINT_DEBUG("Arc is a loop.")
        return;
    }

    Vertex *tail= a->getTail();
    Vertex *head = a->getHead();

    if (head == source) {
        PRINT_DEBUG("Head is source.")
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

   IF_DEBUG(
    if (!checkTree()) {
        std::cerr.flush();
        dumpTree(std::cerr);
        std::cerr.flush();
   });
   assert(checkTree());
}

void SimpleESTree::onSourceSet()
{
    cleanup();
}

bool SimpleESTree::query(const Vertex *t)
{
    PRINT_DEBUG("Querying reachability of " << t);
    if (t == source) {
        PRINT_DEBUG("TRUE");
        return true;
    }

    if (!initialized) {
        PRINT_DEBUG("Query in uninitialized state.");
        run();
    }
    PRINT_DEBUG((reachable[t] ? "TRUE" : "FALSE"));
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

void SimpleESTree::dumpTree(std::ostream &os)
{
    if (!initialized) {
        os << "uninitialized" << std::endl;
    }  else {
        diGraph->mapVertices([&](Vertex *v) {
          auto vd = data[v];
          os << v << ": L " << vd->level << ", P " << vd->getParent() << '\n';
        });
    }
}

bool SimpleESTree::checkTree()
{
   BreadthFirstSearch<FastPropertyMap> bfs;
   bfs.setStartVertex(source);
   bfs.levelAsValues(true);
   FastPropertyMap<int> levels(-1);
   levels.resetAll(diGraph->getSize());
   bfs.useModifiableProperty(&levels);
   runAlgorithm(bfs, diGraph);

   bool ok = true;
   diGraph->mapVertices([&](Vertex *v) {
       auto bfsLevel = levels[v] < 0 ? SimpleESTree::VertexData::UNREACHABLE : levels[v];
       if (data[v]->level != bfsLevel) {
           std::cerr << "Level mismatch for vertex " << data[v]
                        << ": expected level " << bfsLevel << std::endl;
           ok = false;
       }
   });
   return ok;
}

void SimpleESTree::rerun()
{
    reruns++;
    diGraph->mapVertices([&](Vertex *v) {
        data[v]->reset();
    });
    initialized = false;
    run();
}

void SimpleESTree::restoreTree(SimpleESTree::VertexData *rd)
{
    PriorityQueue queue;
    FastPropertyMap<bool> inQueue(false, "", diGraph->getSize());
    FastPropertyMap<unsigned int> timesInQueue(0U, "", diGraph->getSize());
    queue.push(rd);
    inQueue[rd->vertex] = true;
		timesInQueue[rd->vertex]++;
    PRINT_DEBUG("Initialized queue with " << rd << ".")
    bool limitReached = false;
    auto affected = 0U;
    auto affectedLimit = maxAffectedRatio * VertexData::graphSize;
    auto verticesConsidered = 0U;
    auto arcsConsidered = 0U;

    while (!queue.empty()) {
        IF_DEBUG(printQueue(queue))
        auto vd = queue.bot();
        queue.popBot();
        inQueue.resetToDefault(vd->vertex);
#ifdef COLLECT_PR_DATA
        prVertexConsidered();
#endif
        unsigned int levels = process(diGraph, vd, queue, data, reachable, inQueue, timesInQueue, requeueLimit,
                                      limitReached, maxReQueued, verticesConsidered, arcsConsidered);
        affected++;

#ifdef COLLECT_PR_DATA
        prVerticesConsidered(verticesConsidered);
        prArcsConsidered(arcsConsidered);
        verticesConsidered = 0U;
        arcsConsidered = 0U;
#endif
        if (limitReached || (affected > affectedLimit && !queue.empty())) {
            rerun();
            break;
        } else if (levels > 0U) {
            movesDown++;
            levelIncrease += levels;
            if (levels > maxLevelIncrease) {
                maxLevelIncrease = levels;
            }
        }
    }
    totalAffected += affected;
    if (affected > maxAffected) {
        maxAffected = affected;
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
                     const FastPropertyMap<SimpleESTree::VertexData *> &data,
                     FastPropertyMap<bool> &reachable,
                     FastPropertyMap<bool> &inQueue,
                     FastPropertyMap<unsigned int> &timesInQueue,
                     unsigned int limit, bool &limitReached,
                     unsigned int &maxRequeued,
                     unsigned int &verticesConsidered,
                     unsigned int &arcsConsidered) {
#ifndef COLLECT_PR_DATA
    (void)verticesConsidered;
    (void)arcsConsidered;
#endif

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
#ifdef COLLECT_PR_DATA
            arcsConsidered++;
#endif
        if (a->isLoop()) {
            PRINT_DEBUG( "Loop ignored.");
            return;
        }
        auto pd = data(a->getTail());
#ifdef COLLECT_PR_DATA
            verticesConsidered++;
#endif
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
        graph->mapOutgoingArcsUntil(vd->vertex, [&](Arc *a) {
#ifdef COLLECT_PR_DATA
            arcsConsidered++;
#endif
            if (a->isLoop()) {
                return;
            }
            Vertex *head = a->getHead();
            auto *hd = data(head);
#ifdef COLLECT_PR_DATA
            verticesConsidered++;
#endif
            if (hd->isParent(vd) && !inQueue[head]) {
                if (timesInQueue[head] < limit) {
                    PRINT_DEBUG("    Adding child " << hd << " to queue...");
                    timesInQueue[head]++;
                    if (timesInQueue[head] > maxRequeued) {
                        maxRequeued = timesInQueue[head];
                    }
                    queue.push(hd);
                    inQueue[head] = true;
                } else {
                    timesInQueue[head]++;
                    limitReached = true;
                    PRINT_DEBUG("Limit reached for vertex " << head << ".");
                }
            }
        }, [&limitReached](const Arc *) { return limitReached; });
    }

    return levelDiff;
}

}
