/**
 * Copyright (C) 2013 - 2019 : Kathrin Hanauer
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

#ifdef DEBUG_SIMPLEESTREE
void printQueue(boost::circular_buffer<SESVertexData*> q) {
    std::cerr << "PriorityQueue: ";
    while(!q.empty()) {
        std::cerr << q.front()->vertex << "[" << q.bot()->level << "]" << ", ";
        q.pop_front();
    }
    std::cerr << std::endl;
}
#endif


SimpleESTree::SimpleESTree(unsigned int requeueLimit, double maxAffectedRatio)
    : DynamicSSReachAlgorithm(), root(nullptr),
      initialized(false), requeueLimit(requeueLimit),
      maxAffectedRatio(maxAffectedRatio),
			movesDown(0U), movesUp(0U),
      levelIncrease(0U), levelDecrease(0U),
      maxLevelIncrease(0U), maxLevelDecrease(0U),
      decUnreachableHead(0U), decNonTreeArc(0U),
      incUnreachableTail(0U), incNonTreeArc(0U),
      reruns(0U), maxReQueued(0U),
      maxAffected(0U), totalAffected(0U),
      rerunRequeued(0U), rerunNumAffected(0U)
{
    data.setDefaultValue(nullptr);
    reachable.setDefaultValue(false);

    timesInQueue.setDefaultValue(0U);
}

SimpleESTree::~SimpleESTree()
{
    cleanup();
}

DiGraph::size_type SimpleESTree::getDepthOfBFSTree() const
{
	DiGraph::size_type maxLevel = 0U;
    diGraph->mapVertices([&](Vertex *v) {
        if (reachable(v) && data(v)->level > maxLevel) {
            maxLevel = data(v)->level;
        }
    });
    return maxLevel;
}

DiGraph::size_type SimpleESTree::getNumReachable() const
{
	DiGraph::size_type numR = 0U;
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

   BreadthFirstSearch<FastPropertyMap,false> bfs(false);
   root = source;
   if (root == nullptr) {
       root = diGraph->getAnyVertex();
   }
   bfs.setStartVertex(root);
   if (data[root] == nullptr) {
      data[root] = new SESVertexData(root, nullptr, nullptr, 0);
   } else {
       data[root]->reset(nullptr, nullptr, 0);
   }
   reachable[root] = true;
   bfs.onTreeArcDiscover([this](Arc *a) {
#ifdef COLLECT_PR_DATA
        prVertexConsidered();
        prArcConsidered();
#endif
        Vertex *t = a->getTail();
        Vertex *h = a->getHead();
        if (data[h] == nullptr) {
            data[h] = new SESVertexData(h, data(t), a);
        } else {
            data[h]->reset(data(t), a);
        }
        reachable[h] = true;
        PRINT_DEBUG( "(" << t << ", " << h << ")" << " is a tree arc.")
#ifdef COLLECT_PR_DATA
        prArcConsidered();
#endif
   });
   runAlgorithm(bfs, diGraph);

   diGraph->mapVertices([this](Vertex *v) {
#ifdef COLLECT_PR_DATA
        prVertexConsidered();
#endif
       if (data(v) == nullptr) {
           data[v] = new SESVertexData(v);
           PRINT_DEBUG( v << " is a unreachable.")
       }
   });

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
    ss << "#reruns because requeue limit reached: " << rerunRequeued << std::endl;
    ss << "#reruns because max. number of affected vertices reached: " << rerunNumAffected << std::endl;
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
    profile.push_back(std::make_pair(std::string("rerun_requeue_limit"), rerunRequeued));
    profile.push_back(std::make_pair(std::string("rerun_max_affected"), rerunNumAffected));
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
    rerunRequeued = 0U;
    rerunNumAffected = 0U;
    data.resetAll(diGraph->getSize());
    reachable.resetAll(diGraph->getSize());
}

void SimpleESTree::onDiGraphUnset()
{
    cleanup();
    DynamicSSReachAlgorithm::onDiGraphUnset();
}

void SimpleESTree::onVertexAdd(Vertex *v)
{
    if (!initialized) {
        return;
    }

    data[v] = new SESVertexData(v);
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

    auto tail = a->getTail();
    auto head = a->getHead();

    if (head == source) {
        PRINT_DEBUG("Head is source.")
        return;
    }

    auto td = data(tail);
    auto hd = data(head);

    assert(td != nullptr);
    assert(hd != nullptr);

    if (!td->isReachable()) {
        PRINT_DEBUG("Tail is unreachable.")
#ifdef COLLECT_PR_DATA
        incUnreachableTail++;
#endif
        return;
    }

    auto n = diGraph->getSize();

    //update...
    if (hd->level <= td->level + 1) {
        // arc does not change anything
#ifdef COLLECT_PR_DATA
        incNonTreeArc++;
#endif
        PRINT_DEBUG("Not a tree arc.")
        return;
    } else {
        PRINT_DEBUG("Is a new tree arc.")
#ifdef COLLECT_PR_DATA
        movesUp++;
        if (!hd->isReachable()) {
            levelDecrease += (n -  (td->level + 1));
        } else {
            levelDecrease += (hd->level - (td->level + 1));
        }
#endif
        hd->setParent(td, a);
        reachable[head] = true;
    }

    BreadthFirstSearch<FastPropertyMap,false> bfs(false);
    bfs.setStartVertex(head);
    bfs.onArcDiscover([this,&n](const Arc *a) {
        PRINT_DEBUG( "Discovering arc (" << a->getTail() << ", " << a->getHead() << ")...");
#ifdef COLLECT_PR_DATA
        prArcConsidered();
#endif
        if (a->isLoop()) {
            PRINT_DEBUG( "Loop ignored.");
            return false;
        }
        auto at = a->getTail();
        auto ah = a->getHead();
        auto atd = data(at);
        auto ahd = data(ah);

#ifdef COLLECT_PR_DATA
        prVertexConsidered();
#endif
        if (!ahd->isReachable() ||  atd->level + 1 < ahd->level) {
#ifdef COLLECT_PR_DATA
            movesUp++;
            auto newLevel = atd->level + 1;
            auto dec = n - newLevel;
            if (ahd->isReachable()) {
                dec = ahd->level - newLevel;
            }
            levelDecrease += dec;
            if (dec > maxLevelDecrease) {
                maxLevelDecrease = dec;
            }
#endif
            ahd->setParent(atd, const_cast<Arc*>(a));
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
    if (!initialized) {
        return;
    }

     auto vd = data(v);
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

    auto head = a->getHead();

    if (head == source) {
        PRINT_DEBUG("Head is source.")
        return;
    }


    PRINT_DEBUG("Stored data of tail: " << data(a->getTail()));
    PRINT_DEBUG("Stored data of head: " << data(head));

    auto hd = data(head);
    if (hd == nullptr) {
        PRINT_DEBUG("Head of arc is unreachable (and never was). Nothing to do.")
        throw std::logic_error("Should not happen");
    }

    if (!hd->isReachable()) {
        PRINT_DEBUG("Head of arc is already unreachable. Nothing to do.")
#ifdef COLLECT_PR_DATA
        decUnreachableHead++;
#endif
        return;
    }

    if (hd->isTreeArc(a)) {
        hd->parent = nullptr;
        hd->treeArc = nullptr;
        restoreTree(hd);
    } else {
        PRINT_DEBUG("Arc is not a tree arc. Nothing to do.")
#ifdef COLLECT_PR_DATA
        decNonTreeArc++;
#endif
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

std::vector<Arc *> SimpleESTree::queryPath(const Vertex *t)
{
    std::vector<Arc*> path;
    if (!query(t) || t == source) {
        return path;
    }

    while (t != source) {
        auto *a = data(t)->getTreeArc();
        path.push_back(a);
        t = a->getTail();
    }
    assert(!path.empty());

    std::reverse(path.begin(), path.end());

    return path;
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
          os << v << ": L " << vd->level << ", A " << vd->getTreeArc() << ", P " << vd->getParent() << '\n';
        });
    }
}

bool SimpleESTree::checkTree()
{
   BreadthFirstSearch<FastPropertyMap> bfs;
   bfs.setStartVertex(source);
   bfs.levelAsValues(true);
   FastPropertyMap<DiGraph::size_type> levels(bfs.INF);
   levels.resetAll(diGraph->getSize());
   bfs.useModifiableProperty(&levels);
   runAlgorithm(bfs, diGraph);

   bool ok = true;
   diGraph->mapVertices([&](Vertex *v) {
       auto bfsLevel = levels[v] == bfs.INF ? SESVertexData::UNREACHABLE : levels[v];
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
#ifdef COLLECT_PR_DATA
    reruns++;
#endif
    diGraph->mapVertices([&](Vertex *v) {
        data[v]->reset();
    });
    initialized = false;
    run();
}

DiGraph::size_type SimpleESTree::process(SESVertexData *vd, PriorityQueue &queue,
                     bool &limitReached) {

    if (vd->level == 0UL) {
        PRINT_DEBUG("No need to process source vertex " << vd << ".");
        return 0U;
    }

#ifdef COLLECT_PR_DATA
    auto verticesConsidered = 0U;
    auto arcsConsidered = 0U;
#endif

    PRINT_DEBUG("Processing vertex " << vd << ".");
    Vertex *v = vd->getVertex();

    auto oldParent = vd->getParentData();

    if (vd->hasValidParent()) {
        PRINT_DEBUG("Parent still valid. No further processing required.");
        return 0U;
    } else if (!vd->isReachable()) {
        PRINT_DEBUG("Vertex is already unreachable.");
        return 0U;
    }

    auto parent = oldParent;
    auto oldVLevel = vd->level;
    auto minParentLevel = parent == nullptr ? SESVertexData::UNREACHABLE : parent->level;
    auto treeArc = vd->treeArc;

    PRINT_DEBUG("Min parent level is " << minParentLevel << ".");

    diGraph->mapIncomingArcsUntil(v, [this,&parent,&minParentLevel,&oldVLevel,&treeArc](Arc *a) {
#ifdef COLLECT_PR_DATA
            prArcConsidered();
#endif
        if (a->isLoop()) {
            PRINT_DEBUG( "Loop ignored.");
            return;
        }
        auto pd = data(a->getTail());
#ifdef COLLECT_PR_DATA
            prVertexConsidered();
#endif
        auto pLevel = pd->level;
        if (pLevel < minParentLevel) {
            minParentLevel = pLevel;
            parent = pd;
            treeArc = a;
            PRINT_DEBUG("Update: Min parent level now is " << minParentLevel << ", parent " << parent);
            assert (minParentLevel + 1 >= oldVLevel);
        }
    }, [&oldVLevel, &minParentLevel](const Arc *) { return minParentLevel + 1 == oldVLevel; });

		DiGraph::size_type levelDiff = 0U;
    auto n = diGraph->getSize();

    if ((parent == nullptr || minParentLevel >= n - 1)
            && vd->isReachable()) {
        vd->setUnreachable();
        reachable.resetToDefault(v);
        levelDiff = n - oldVLevel;
        PRINT_DEBUG("No parent or parent is unreachable. Vertex is unreachable. Level diff " << levelDiff);
    } else if (parent != oldParent || oldVLevel <= minParentLevel) {
        assert(parent->isReachable());
        assert(minParentLevel != SESVertexData::UNREACHABLE);
        vd->setParent(parent, treeArc);
        assert (vd->level >= oldVLevel);
        levelDiff = vd->level - oldVLevel;
        PRINT_DEBUG("Parent has changed, new parent is " << parent);
    }

    if (levelDiff > 0U) {
        PRINT_DEBUG("Updating children...");
        diGraph->mapOutgoingArcsUntil(vd->vertex, [this,&queue,&limitReached](Arc *a) {
#ifdef COLLECT_PR_DATA
            prArcConsidered();
#endif
            if (a->isLoop()) {
                return;
            }
            Vertex *head = a->getHead();
            auto *hd = data(head);
#ifdef COLLECT_PR_DATA
            prVertexConsidered();
#endif
            if (hd->isTreeArc(a)) {
                if (timesInQueue[head] < requeueLimit) {
                    PRINT_DEBUG("    Adding child " << hd << " to queue...");
                    timesInQueue[head]++;
                    if (timesInQueue[head] > maxReQueued) {
                        maxReQueued = timesInQueue[head];
                    }
                    queue.push_back(hd);
                } else {
                    timesInQueue[head]++;
                    limitReached = true;
                    PRINT_DEBUG("Limit reached for vertex " << head << ".");
                }
            }
        }, [&limitReached](const Arc *) { return limitReached; });
    }

#ifdef COLLECT_PR_DATA
        prVerticesConsidered(verticesConsidered);
        prArcsConsidered(arcsConsidered);
#endif

    return levelDiff;
}

void SimpleESTree::restoreTree(SESVertexData *rd)
{
    PriorityQueue queue;
    queue.set_capacity(diGraph->getSize());
    timesInQueue.resetAll(diGraph->getSize());
    queue.push_back(rd);
    timesInQueue[rd->getVertex()]++;
    if (maxReQueued == 0U) {
        maxReQueued = 1U;
    }
    PRINT_DEBUG("Initialized queue with " << rd << ".")
    bool limitReached = false;
    auto processed = 0ULL;
    auto affectedLimit = maxAffectedRatio * diGraph->getSize();

    while (!queue.empty()) {
        IF_DEBUG(printQueue(queue))
        auto vd = queue.front();
        queue.pop_front();
#ifdef COLLECT_PR_DATA
        prVertexConsidered();
        auto levels =
#endif
        process(vd, queue, limitReached);
        processed++;

        if (limitReached || ((processed + queue.size() > affectedLimit) && !queue.empty())) {
#ifdef COLLECT_PR_DATA
            if (limitReached) {
                rerunRequeued++;
            }
            if ((processed + queue.size() > affectedLimit) && !queue.empty()) {
                rerunNumAffected++;
            }
#endif
            rerun();
            break;
#ifdef COLLECT_PR_DATA
        } else if (levels > 0U) {
            movesDown++;
            levelIncrease += levels;
            if (levels > maxLevelIncrease) {
                maxLevelIncrease = levels;
            }
#endif
        }
    }
#ifdef COLLECT_PR_DATA
    totalAffected += processed;
    if (processed > maxAffected) {
        maxAffected = processed;
    }
#endif
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

}
