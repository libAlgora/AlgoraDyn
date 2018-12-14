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

#include "estree-bqueue.h"

#include <vector>
#include <climits>
#include <cassert>

#include "graph/vertex.h"
#include "algorithm.basic.traversal/breadthfirstsearch.h"
#include "algorithm/digraphalgorithmexception.h"
#include "property/fastpropertymap.h"

//#define DEBUG_OLDESTREE

#ifdef DEBUG_OLDESTREE
#include <iostream>
#define PRINT_DEBUG(msg) std::cerr << msg << std::endl;
#define IF_DEBUG(cmd) cmd;
#else
#define PRINT_DEBUG(msg)
#define IF_DEBUG(cmd)
#endif

namespace Algora {

#ifdef DEBUG_OLDESTREE
void printQueue(PriorityQueue q) {
    std::cerr << "PriorityQueue: ";
    while(!q.empty()) {
        std::cerr << q.bot()->vertex << "[" << q.bot()->level << "]" << ", ";
        q.popBot();
    }
    std::cerr << std::endl;
}
#endif

OldESTree::OldESTree(unsigned long long requeueLimit, double maxAffectedRatio)
    : DynamicSSReachAlgorithm(), root(nullptr),
      initialized(false), requeueLimit(requeueLimit),
      maxAffectedRatio(maxAffectedRatio),
      movesDown(0U), movesUp(0U),
      levelIncrease(0U), levelDecrease(0U),
      maxLevelIncrease(0U), maxLevelDecrease(0U),
      decUnreachableHead(0U), decNonTreeArc(0U),
      incUnreachableTail(0U), incNonTreeArc(0U),
      reruns(0U), maxReQueued(0U),
      maxAffected(0U), totalAffected(0U)
{
    data.setDefaultValue(nullptr);
    inNeighborIndices.setDefaultValue(0U);
    reachable.setDefaultValue(false);
}

OldESTree::~OldESTree()
{
    cleanup();
}

void OldESTree::run()
{
    if (initialized) {
        return;
    }

   PRINT_DEBUG("Initializing OldESTree...")

   reachable.resetAll(diGraph->getSize());

   BreadthFirstSearch<FastPropertyMap> bfs(false);
   root = source;
   if (root == nullptr) {
       root = diGraph->getAnyVertex();
   }
   bfs.setStartVertex(root);
   if (data[root] == nullptr) {
      data[root] = new ESVertexData(&inNeighborIndices, root, nullptr, nullptr, 0U);
   } else {
       data[root]->reset(nullptr, nullptr, 0U);
   }
   reachable[root] = true;
   bfs.onTreeArcDiscover([&](Arc *a) {
#ifdef COLLECT_PR_DATA
        prVertexConsidered();
        prArcConsidered();
#endif
        Vertex *t = a->getTail();
        Vertex *h = a->getHead();
        PRINT_DEBUG( "(" << t << ", " << h << ")" << " is a tree arc.")
        if (data[h] == nullptr) {
            data[h] = new ESVertexData(&inNeighborIndices, h, data(t), a, 0U);
        } else {
            data[h]->reset(data(t), a);
        }
        reachable[h] = true;
   });
   bfs.onNonTreeArcDiscover([&](Arc *a) {
        if (a->isLoop() || a->getHead() == source) {
            return;
        }
#ifdef COLLECT_PR_DATA
        prArcConsidered();
#endif
        Vertex *t = a->getTail();
        Vertex *h = a->getHead();
        ESVertexData *td = data(t);
        ESVertexData *hd = data(h);
        PRINT_DEBUG( "(" << td->vertex << ", " << hd->vertex << ")" << " is a non-tree arc.")
        hd->addInNeighbor(td, a);
   });
   runAlgorithm(bfs, diGraph);

   diGraph->mapArcs([&](Arc *a) {
#ifdef COLLECT_PR_DATA
        prArcConsidered();
#endif
       if (a->isLoop() || a->getHead() == source) {
           return;
       }
       Vertex *t = a->getTail();
       Vertex *h = a->getHead();
       ESVertexData *td = data(t);
       ESVertexData *hd = data(h);

       if (td == nullptr) {
           td = new ESVertexData(&inNeighborIndices, t);
           data[t] = td;
       }
       if (hd == nullptr) {
           hd = new ESVertexData(&inNeighborIndices, h);
           data[h] = hd;
       }
       if (!td->isReachable()) {
            PRINT_DEBUG( "(" << td->vertex << ", " << hd->vertex << ")" << " is an unvisited non-tree arc.")
            hd->addInNeighbor(td, a);
       }
   });

   diGraph->mapVertices([&](Vertex *v) {
#ifdef COLLECT_PR_DATA
        prVertexConsidered();
#endif
       if (data(v) == nullptr) {
           data[v] = new ESVertexData(&inNeighborIndices, v);
           PRINT_DEBUG( v << " is unreachable.")
       }
   });

   initialized = true;
   PRINT_DEBUG("Initializing completed.")

   IF_DEBUG(
    if (!checkTree()) {
        std::cerr.flush();
        dumpTree(std::cerr);
        std::cerr.flush();
    });
   assert(checkTree());
}

std::string OldESTree::getProfilingInfo() const
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

DynamicSSReachAlgorithm::Profile OldESTree::getProfile() const
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

void OldESTree::onDiGraphSet()
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
}

void OldESTree::onDiGraphUnset()
{
    cleanup();
    DynamicSSReachAlgorithm::onDiGraphUnset();
}

void OldESTree::onVertexAdd(Vertex *v)
{
    data[v] = new ESVertexData(&inNeighborIndices, v);
}

void OldESTree::onArcAdd(Arc *a)
{
    if (!initialized) {
        return;
    }
    PRINT_DEBUG("An arc has been added: (" << a->getTail() << ", " << a->getHead() << ")")

    Vertex *tail = a->getTail();
    Vertex *head = a->getHead();

    IF_DEBUG(
        std::stringstream ss;
        dumpTree(ss);
    )

    if (a->isLoop()) {
        PRINT_DEBUG("Arc is a loop.")
        return;
    }

    if (head == source) {
        PRINT_DEBUG("Head is source.")
        return;
    }

    ESVertexData *td = data(tail);
    ESVertexData *hd = data(head);

    assert(td != nullptr);
    assert(hd != nullptr);

    // store arc
    hd->addInNeighbor(td, a);

    if (!td->isReachable()) {
        PRINT_DEBUG("Tail is unreachable.")
        incUnreachableTail++;
        return;
    }

    //update...
    auto diff = hd->reparent(td, a);
    if (diff == 0U) {
        // arc does not change anything
        PRINT_DEBUG("Does not decrease level.")
        incNonTreeArc++;
        return;
    } else {
        PRINT_DEBUG("Is a new tree arc, diff is " << diff);
        movesUp++;
        reachable[head] = true;
    }

    BreadthFirstSearch<FastPropertyMap> bfs(false);
    bfs.setStartVertex(head);
    bfs.onArcDiscover([&](const Arc *a) {
        PRINT_DEBUG( "Discovering arc (" << a->getTail() << ", " << a->getHead() << ")...");
#ifdef COLLECT_PR_DATA
        prArcConsidered();
#endif
        if (a->isLoop()) {
            return false;
        }
        Vertex *at = a->getTail();
        Vertex *ah = a->getHead();
        ESVertexData *atd = data(at);
        ESVertexData *ahd = data(ah);

        auto diff = ahd->reparent(atd, a);
#ifdef COLLECT_PR_DATA
        prVertexConsidered();
#endif
        if (diff > 0U) {
            PRINT_DEBUG("Is a new tree arc.");
            movesUp++;
            reachable[ah] = true;
            if (diff > maxLevelDecrease) {
                if (diff - diGraph->getSize() > 0) {
                    diff -= (ESVertexData::UNREACHABLE - diGraph->getSize());
                }
                maxLevelDecrease = diff;
            }
        }
        return diff > 0U;

    });
    runAlgorithm(bfs, diGraph);

    IF_DEBUG(
    if (!checkTree()) {
        std::cerr << "Tree before:" << std::endl;
        std::cerr << ss.rdbuf();
        std::cerr << "Tree after:" << std::endl;
        dumpTree(std::cerr);
        std::cerr.flush();
    });
   assert(checkTree());
}

void OldESTree::onVertexRemove(Vertex *v)
{
    if (!initialized) {
        return;
    }

     ESVertexData *vd = data(v);
     if (vd != nullptr) {
         delete vd;
         data.resetToDefault(v);
         reachable.resetToDefault(v);
     }
}

void OldESTree::onArcRemove(Arc *a)
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

    if (head == source) {
        PRINT_DEBUG("Head is source.")
        return;
    }

    PRINT_DEBUG("An arc is about to be removed: (" << tail << ", " << head << ")");

    PRINT_DEBUG("Stored data of tail: " << data(tail));
    PRINT_DEBUG("Stored data of head: " << data(head));

    IF_DEBUG(
        std::stringstream ss;
        dumpTree(ss);
    );

    ESVertexData *hd = data(head);
    if (hd == nullptr) {
        PRINT_DEBUG("Head of arc is unreachable (and never was). Nothing to do.")
        throw std::logic_error("Should not happen");
        return;
    }

    ESVertexData *td = data(tail);
    bool isParent = hd->isParent(td);
    hd->findAndRemoveInNeighbor(td, a);

    if (!hd->isReachable()) {
        PRINT_DEBUG("Head of arc is already unreachable. Nothing to do.")
        decUnreachableHead++;
        return;
    }

    if (hd->level <= td->level || !isParent) {
        PRINT_DEBUG("Arc is not a tree arc. Nothing to do.");
        decNonTreeArc++;
    } else {
        restoreTree(hd);
    }

    IF_DEBUG(
    if (!checkTree()) {
        std::cerr << "Tree before:" << std::endl;
        std::cerr << ss.rdbuf();
        std::cerr << "Tree after:" << std::endl;
        dumpTree(std::cerr);
        std::cerr.flush();
    });
   assert(checkTree());
}

void OldESTree::onSourceSet()
{
    cleanup();
}

bool OldESTree::query(const Vertex *t)
{
    if (t == source) {
        return true;
    }

    if (!initialized) {
        run();
    }
    assert(checkTree());
    return reachable(t);
}

void OldESTree::dumpData(std::ostream &os)
{
    if (!initialized) {
        os << "uninitialized" << std::endl;
    }  else {
        for (auto i = data.cbegin(); i != data.cend(); i++) {
            os << (*i) << std::endl;
        }
    }
}

void OldESTree::dumpTree(std::ostream &os)
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

bool OldESTree::checkTree()
{
   BreadthFirstSearch<FastPropertyMap> bfs;
   bfs.setStartVertex(source);
   bfs.levelAsValues(true);
   FastPropertyMap<unsigned long long> levels(bfs.INFINITY);
   levels.resetAll(diGraph->getSize());
   bfs.useModifiableProperty(&levels);
   runAlgorithm(bfs, diGraph);

   bool ok = true;
   diGraph->mapVertices([&](Vertex *v) {
       auto bfsLevel = levels[v] == bfs.INFINITY ? ESVertexData::UNREACHABLE : levels[v];
       if (data[v]->level != bfsLevel) {
           std::cerr << "Level mismatch for vertex " << data[v]
                        << ": expected level " << bfsLevel << std::endl;
           ok = false;
       }
       if (!data[v]->checkIntegrity()) {
           std::cerr << "Integrity check failed for vertex " << data[v] << std::endl;
           ok = false;
       }
       if (reachable(v) != data[v]->isReachable()) {
           std::cerr << "Reachability flag diverges from state according to BFS tree: " <<
                     reachable(v) << " vs " << data[v] << std::endl;
           ok = false;
       }
   });
   return ok;
}

void OldESTree::rerun()
{
    reruns++;
    diGraph->mapVertices([&](Vertex *v) {
        data[v]->reset();
    });
    initialized = false;
    run();
}

unsigned long long OldESTree::process(ESVertexData *vd, PriorityQueue &queue, FastPropertyMap<bool> &inQueue, FastPropertyMap<unsigned long long> &timesInQueue, bool &limitReached)
{
    if (vd->getLevel() == 0ULL) {
        PRINT_DEBUG("No need to process source vertex " << vd << ".");
        return 0U;
    }

    PRINT_DEBUG("Processing vertex " << vd << ".");

    if (!vd->isReachable()) {
        PRINT_DEBUG("vertex is already unreachable.");
        return 0U;
    }

#ifdef COLLECT_PR_DATA
        auto verticesConsidered = 0U;
        auto arcsConsidered = 0U;
#endif

    Vertex *v = vd->getVertex();
    bool reachV = true;
    bool levelChanged = false;
    auto n = diGraph->getSize();
    auto oldVLevel = vd->getLevel();
    auto levelDiff = 0ULL;

    auto enqueue = [&](ESVertexData *vd) {
        auto vertex = vd->getVertex();
        if (timesInQueue[vertex] < requeueLimit) {
            PRINT_DEBUG("    Adding " << vd << " to queue...");
            timesInQueue[vertex]++;
            if (timesInQueue[vertex] > maxReQueued) {
                maxReQueued = timesInQueue[vertex];
            }
            queue.push(vd);
            inQueue[vertex] = true;
        } else {
            timesInQueue[vertex]++;
            limitReached = true;
            PRINT_DEBUG("Limit reached for vertex " << vertex << ".");
        }
    };

    if (vd->inNeighbors.empty()) {
        PRINT_DEBUG("Vertex is a source.");
        if (reachV) {
            reachV = false;
            vd->setUnreachable();
            reachable[v] = false;
            levelChanged = true;
            levelDiff = n - oldVLevel;
            PRINT_DEBUG("Level changed.");
        }
    } else {
        auto *parent = vd->getParentData();

        PRINT_DEBUG("Size of graph is " << n << ".");
        PRINT_DEBUG("Parent is " << parent);

        while (reachV && (parent == nullptr || vd->level <= parent->level) && !levelChanged) {
#ifdef COLLECT_PR_DATA
            verticesConsidered++;

#endif
            vd->parentIndex++;
            PRINT_DEBUG("  Advancing parent index to " << vd->parentIndex << ".")

            if (vd->parentIndex >= vd->inNeighbors.size()) {
                if (vd->level + 1 >= diGraph->getSize()) {
                    PRINT_DEBUG("    Vertex " << v << " is unreachable.")
                    vd->setUnreachable();
                    reachable.resetToDefault(v);
                    reachV = false;
                    levelChanged = true;
                    levelDiff = n - oldVLevel;
                } else {
                    vd->level++;
                    levelDiff++;
                    levelChanged = true;
                    PRINT_DEBUG("  Maximum parent index exceeded, increasing level to " << vd->level << ".")
                    vd->parentIndex = 0;
                }
            }
            if (reachV && !levelChanged)  {
                parent = vd->getParentData();
                PRINT_DEBUG("  Trying " << parent << " as parent.")
            }
        }
    }
    if (levelChanged) {
        diGraph->mapOutgoingArcsUntil(vd->getVertex(), [&](Arc *a) {
#ifdef COLLECT_PR_DATA
            arcsConsidered++;
#endif
            if (a->isLoop()) {
              PRINT_DEBUG("    Ignoring loop.");
              return;
            }
            Vertex *head = a->getHead();
#ifdef COLLECT_PR_DATA
            verticesConsidered++;
#endif
            auto *hd = data(head);
            if (hd->isParent(vd) && !inQueue[head]) {
                enqueue(hd);
            } else {
              PRINT_DEBUG("    NOT adding " << hd << " to queue: not a child of " << vd)
            }
        }, [&limitReached](const Arc *) { return limitReached; });
        if (reachV && !limitReached) {
            enqueue(vd);
        }
    }

    PRINT_DEBUG("Returning level diff " << levelDiff  << " for " << vd << ".");

    //assert(vd->checkIntegrity());
#ifdef COLLECT_PR_DATA
        prVerticesConsidered(verticesConsidered);
        prArcsConsidered(arcsConsidered);
#endif

    return levelDiff;
}

void OldESTree::restoreTree(ESVertexData *vd)
{
    PriorityQueue queue;
    queue.setLimit(diGraph->getSize());
    FastPropertyMap<bool> inQueue(false, "", diGraph->getSize());
    FastPropertyMap<unsigned long long> timesInQueue(0ULL, "", diGraph->getSize());
    queue.push(vd);
    inQueue[vd->getVertex()] = true;
    timesInQueue[vd->getVertex()]++;
    PRINT_DEBUG("Initialized queue with " << vds.size() << " vertices.");
    bool limitReached = false;
    auto affected = 0U;
    auto affectedLimit = maxAffectedRatio * diGraph->getSize();

    while (!queue.empty()) {
        IF_DEBUG(printQueue(queue))
        auto vd = queue.bot();
        queue.popBot();
        inQueue[vd->getVertex()] = false;
#ifdef COLLECT_PR_DATA
        prVertexConsidered();
#endif
        auto levels = process(vd, queue, inQueue, timesInQueue, limitReached);
        affected++;

        if (limitReached || (affected > affectedLimit && !queue.empty())) {
            rerun();
            break;
        } else if (levels > 0U) {
            movesDown++;
            levelIncrease += levels;
            PRINT_DEBUG("total level increase " << levelIncrease);
            if (levels > maxLevelIncrease) {
                maxLevelIncrease = levels;
                PRINT_DEBUG("new max level increase " << maxLevelIncrease);
            }
        }
    }
    totalAffected += affected;
    if (affected > maxAffected) {
        maxAffected = affected;
    }
}

void OldESTree::cleanup()
{
    for (auto i = data.cbegin(); i != data.cend(); i++) {
        if ((*i)) {
            delete (*i);
        }
    }

    data.resetAll();
    reachable.resetAll();
    inNeighborIndices.resetAll();

    initialized = false ;

}

}
