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

#include "estree-ml.h"

#include <vector>
#include <climits>
#include <cassert>

#include "graph/vertex.h"
#include "algorithm.basic.traversal/breadthfirstsearch.h"
#include "algorithm/digraphalgorithmexception.h"

//#define DEBUG_ESTREEML

#ifdef DEBUG_ESTREEML
#include <iostream>
#define PRINT_DEBUG(msg) std::cerr << "MES(" << getSource() << "): " << msg << std::endl;
#define IF_DEBUG(cmd) cmd;
#else
#define PRINT_DEBUG(msg)
#define IF_DEBUG(cmd)
#endif

namespace Algora {

#ifdef DEBUG_ESTREEML
void printQueue(boost::circular_buffer<ESVertexData*> q) {
    std::cerr << "PriorityQueue: ";
    while(!q.empty()) {
        std::cerr << q.front()->getVertex() << "[" << q.front()->getLevel() << "]" << ", ";
        q.pop_front();
    }
    std::cerr << std::endl;
}
#endif

template<bool reverseArcDirection>
ESTreeML<reverseArcDirection>::ESTreeML(unsigned int requeueLimit, double maxAffectedRatio)
    : ESTreeML<reverseArcDirection>(std::make_pair(requeueLimit, maxAffectedRatio))
{
}

template<bool reverseArcDirection>
ESTreeML<reverseArcDirection>::ESTreeML(const ESTreeML<reverseArcDirection>::ParameterSet &params)
    : DynamicSingleSourceReachabilityAlgorithm(), root(nullptr),
      initialized(false),
      requeueLimit(std::get<0>(params)),
      maxAffectedRatio(std::get<1>(params)),
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
    inNeighborIndices.setDefaultValue(0U);
    reachable.setDefaultValue(false);

    timesInQueue.setDefaultValue(0U);
}

template<bool reverseArcDirection>
ESTreeML<reverseArcDirection>::~ESTreeML()
{
    cleanup(true);
}

template<bool reverseArcDirection>
void ESTreeML<reverseArcDirection>::run()
{
    if (initialized) {
        return;
    }

   PRINT_DEBUG("Initializing ESTreeML...");

   if (reachable.size() < diGraph->getSize()) {
       reachable.resetAll(diGraph->getSize());
   } else {
       reachable.resetAll();
   }
   if (inNeighborIndices.size() < diGraph->getNumArcs(true)) {
       inNeighborIndices.resetAll(diGraph->getNumArcs(true));
   } else {
       inNeighborIndices.resetAll();
   }

   BreadthFirstSearch<FastPropertyMap,false,reverseArcDirection> bfs(false);
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
   bfs.onTreeArcDiscover([this](Arc *a) {
#ifdef COLLECT_PR_DATA
        prVertexConsidered();
        prArcConsidered();
#endif
        Vertex *t;
        Vertex *h;
        if (reverseArcDirection) {
            t = a->getHead();
            h = a->getTail();
        } else {
            t = a->getTail();
            h = a->getHead();
        }
        PRINT_DEBUG( a << " is a tree arc.")
        if (data[h] == nullptr) {
            data[h] = new ESVertexData(&inNeighborIndices, h, data(t), a, 0U);
        } else {
            data[h]->reset(data(t), a);
        }
        reachable[h] = true;
   });
   bfs.onNonTreeArcDiscover([this](Arc *a) {
        if (a->isLoop()
                || (!reverseArcDirection && a->getHead() == source)
                || (reverseArcDirection && a->getTail() == source)) {
            return;
        }
#ifdef COLLECT_PR_DATA
        prArcConsidered();
#endif
        Vertex *t;
        Vertex *h;
        if (reverseArcDirection) {
            t = a->getHead();
            h = a->getTail();
        } else {
            t = a->getTail();
            h = a->getHead();
        }
        ESVertexData *td = data(t);
        ESVertexData *hd = data(h);
        PRINT_DEBUG( a << " is a non-tree arc.")
        hd->addInNeighbor(td, a);
   });
   runAlgorithm(bfs, diGraph);

   diGraph->mapArcs([this](Arc *a) {
#ifdef COLLECT_PR_DATA
        prArcConsidered();
#endif
       if (a->isLoop()
                || (!reverseArcDirection && a->getHead() == source)
                || (reverseArcDirection && a->getTail() == source)) {
           return;
       }
       Vertex *t;
       Vertex *h;
       if (reverseArcDirection) {
           t = a->getHead();
           h = a->getTail();
       } else {
           t = a->getTail();
           h = a->getHead();
       }
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
            PRINT_DEBUG( a << " is an unvisited non-tree arc.")
            hd->addInNeighbor(td, a);
       }
   });

   diGraph->mapVertices([this](Vertex *v) {
#ifdef COLLECT_PR_DATA
        prVertexConsidered();
#endif
       if (data(v) == nullptr) {
           data[v] = new ESVertexData(&inNeighborIndices, v);
           PRINT_DEBUG( v << " is unreachable.")
       }
   });

   initialized = true;
   PRINT_DEBUG("Initialization completed.")

   IF_DEBUG(
    if (!checkTree()) {
        std::cerr.flush();
        dumpTree(std::cerr);
        std::cerr.flush();
    });
   assert(checkTree());
}

template<bool reverseArcDirection>
std::string ESTreeML<reverseArcDirection>::getProfilingInfo() const
{
    std::stringstream ss;
#ifdef COLLECT_PR_DATA
    ss << DynamicSingleSourceReachabilityAlgorithm::getProfilingInfo();
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
#endif
    return ss.str();
}

template<bool reverseArcDirection>
DynamicSingleSourceReachabilityAlgorithm::Profile ESTreeML<reverseArcDirection>::getProfile() const
{
    auto profile = DynamicSingleSourceReachabilityAlgorithm::getProfile();
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

template<bool reverseArcDirection>
void ESTreeML<reverseArcDirection>::onDiGraphSet()
{
    DynamicSingleSourceReachabilityAlgorithm::onDiGraphSet();
    cleanup(false);

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
}

template<bool reverseArcDirection>
void ESTreeML<reverseArcDirection>::onDiGraphUnset()
{
    DynamicSingleSourceReachabilityAlgorithm::onDiGraphUnset();
    cleanup(true);
}

template<bool reverseArcDirection>
void ESTreeML<reverseArcDirection>::onVertexAdd(Vertex *v)
{
    if (!initialized) {
        return;
    }
    data[v] = new ESVertexData(&inNeighborIndices, v);
}

template<bool reverseArcDirection>
void ESTreeML<reverseArcDirection>::onArcAdd(Arc *a)
{
    if (!initialized) {
        return;
    }
    PRINT_DEBUG("An arc has been added: " << a);

    Vertex *tail;
    Vertex *head;
    if (reverseArcDirection) {
        tail = a->getHead();
        head = a->getTail();
    } else {
        tail = a->getTail();
        head = a->getHead();
    }

    IF_DEBUG(
        std::stringstream ss;
        dumpTree(ss);
    )

    if (a->isLoop()) {
        PRINT_DEBUG("Arc is a loop.")
        return;
    }

    if (head == source) {
        if (reverseArcDirection) {
            PRINT_DEBUG("Tail is source.")
        } else {
            PRINT_DEBUG("Head is source.")
        }
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
#ifdef COLLECT_PR_DATA
        incUnreachableTail++;
#endif
        return;
    }

    //update...
    auto diff = hd->reparent(td, a);
    if (diff == 0U) {
        // arc does not change anything
        PRINT_DEBUG("Does not decrease level.")
#ifdef COLLECT_PR_DATA
        incNonTreeArc++;
#endif
        return;
    } else {
        PRINT_DEBUG("Is a new tree arc, diff is " << diff);
#ifdef COLLECT_PR_DATA
        movesUp++;
#endif
        reachable[head] = true;
    }

    BreadthFirstSearch<FastPropertyMap,false,reverseArcDirection> bfs(false);
    bfs.setStartVertex(head);
    bfs.onArcDiscover([this](const Arc *ca) {
        auto *a = const_cast<Arc*>(ca);
        PRINT_DEBUG( "Discovering arc " << a << "...");
#ifdef COLLECT_PR_DATA
        prArcConsidered();
#endif
        if (a->isLoop()) {
            return false;
        }
        Vertex *at;
        Vertex *ah;
        if (reverseArcDirection) {
            at = a->getHead();
            ah = a->getTail();
        } else {
            at = a->getTail();
            ah = a->getHead();
        }
        auto atd = data(at);
        auto ahd = data(ah);

        auto diff = ahd->reparent(atd, a);
#ifdef COLLECT_PR_DATA
        prVertexConsidered();
#endif
        if (diff > 0U) {
            PRINT_DEBUG("Is a new tree arc.");
            reachable[ah] = true;
#ifdef COLLECT_PR_DATA
            movesUp++;
            if (diff > maxLevelDecrease) {
                if (diff - diGraph->getSize() > 0) {
                    diff -= (ESVertexData::UNREACHABLE - diGraph->getSize());
                }
                maxLevelDecrease = diff;
            }
#endif
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

template<bool reverseArcDirection>
void ESTreeML<reverseArcDirection>::onVertexRemove(Vertex *v)
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

template<bool reverseArcDirection>
void ESTreeML<reverseArcDirection>::onArcRemove(Arc *a)
{
   if (!initialized) {
        return;
    }

    if (a->isLoop()) {
        PRINT_DEBUG("Arc is a loop.")
        return;
    }

    Vertex *tail;
    Vertex *head;
    if (reverseArcDirection) {
        tail = a->getHead();
        head = a->getTail();
    } else {
        tail = a->getTail();
        head = a->getHead();
    }

    if (head == source) {
        if (reverseArcDirection) {
            PRINT_DEBUG("Tail is source.")
        } else {
            PRINT_DEBUG("Head is source.")
        }
        return;
    }

    PRINT_DEBUG("An arc is about to be removed: " << a);

    PRINT_DEBUG("Stored data of tail: " << data(tail));
    PRINT_DEBUG("Stored data of head: " << data(head));

    IF_DEBUG(
        std::stringstream ss;
        dumpTree(ss);
    );

    ESVertexData *hd = data(head);
    assert(hd != nullptr);
    //if (hd == nullptr) {
    //    if (reverseArcDirection) {
    //        PRINT_DEBUG("Tail of arc is unreachable (and never was). Nothing to do.")
    //    } else {
    //        PRINT_DEBUG("Head of arc is unreachable (and never was). Nothing to do.")
    //    }
    //    throw std::logic_error("Should not happen");
    //}

    ESVertexData *td = data(tail);
    bool isParent = hd->isTreeArc(a);
    hd->findAndRemoveInNeighbor(td, a);

    if (!hd->isReachable()) {
        if (reverseArcDirection) {
            PRINT_DEBUG("Tail of arc is already unreachable. Nothing to do.")
        } else {
            PRINT_DEBUG("Head of arc is already unreachable. Nothing to do.")
        }
#ifdef COLLECT_PR_DATA
        decUnreachableHead++;
#endif
        return;
    }

    if (hd->level <= td->level || !isParent) {
        PRINT_DEBUG("Arc is not a tree arc. Nothing to do.");
#ifdef COLLECT_PR_DATA
        decNonTreeArc++;
#endif
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

template<bool reverseArcDirection>
void ESTreeML<reverseArcDirection>::onSourceSet()
{
    cleanup(false);
}

template<bool reverseArcDirection>
bool ESTreeML<reverseArcDirection>::query(const Vertex *t)
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

template<bool reverseArcDirection>
std::vector<Arc *> ESTreeML<reverseArcDirection>::queryPath(const Vertex *t)
{
    std::vector<Arc*> path;
    if (!query(t) || t == source) {
        return path;
    }

    while (t != source) {
        auto *a = data(t)->getTreeArc();
        path.push_back(a);
        t = reverseArcDirection ? a->getHead() : a->getTail();
    }
    assert(!path.empty());

    if (!reverseArcDirection) {
        std::reverse(path.begin(), path.end());
    }

    return path;
}

template<bool reverseArcDirection>
void ESTreeML<reverseArcDirection>::dumpData(std::ostream &os) const
{
    if (!initialized) {
        os << "uninitialized" << std::endl;
    }  else {
        for (auto i = data.cbegin(); i != data.cend(); i++) {
            os << (*i) << std::endl;
        }

        os << "Tree in dot format:\ndigraph MESTree {\n";
        for (const auto vd : data) {
            auto treeArc = vd->getTreeArc();
            if (treeArc) {
                os << treeArc->getTail()->getName() << " -> "
                   << treeArc->getHead()->getName() << ";\n";
            }
        }
        os << "}" << std::endl;
    }
}

template<bool reverseArcDirection>
void ESTreeML<reverseArcDirection>::dumpTree(std::ostream &os)
{
    if (!initialized) {
        os << "uninitialized" << std::endl;
    }  else {
        diGraph->mapVertices([&](Vertex *v) {
          auto vd = data[v];
          os << v << ": L " << vd->getLevel() << ", P " << vd->getParent() << '\n';
        });
    }
}

template<bool reverseArcDirection>
bool ESTreeML<reverseArcDirection>::checkTree()
{
   BreadthFirstSearch<FastPropertyMap,true,reverseArcDirection> bfs;
   bfs.setStartVertex(source);
   bfs.levelAsValues(true);
   FastPropertyMap<DiGraph::size_type> levels(bfs.INF);
   levels.resetAll(diGraph->getSize());
   bfs.useModifiableProperty(&levels);
   runAlgorithm(bfs, diGraph);

   bool ok = true;
   diGraph->mapVertices([&](Vertex *v) {
       auto bfsLevel = levels[v] == bfs.INF ? ESVertexData::UNREACHABLE : levels[v];
       if (data[v]->getLevel() != bfsLevel) {
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

template<bool reverseArcDirection>
void ESTreeML<reverseArcDirection>::rerun()
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

template<bool reverseArcDirection>
DiGraph::size_type ESTreeML<reverseArcDirection>::process(ESVertexData *vd, bool &limitReached)
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

    auto enqueue = [this,&limitReached](ESVertexData *vd) {
        auto vertex = vd->getVertex();
        if (timesInQueue[vertex] < requeueLimit) {
            PRINT_DEBUG("    Adding " << vd << " to queue...");
            timesInQueue[vertex]++;
            if (timesInQueue[vertex] > maxReQueued) {
                maxReQueued = timesInQueue[vertex];
            }
            queue.push_back(vd);
        } else {
            timesInQueue[vertex]++;
            limitReached = true;
            PRINT_DEBUG("Limit reached for vertex " << vertex << ".");
        }
    };

    auto minParentLevel = ESVertexData::UNREACHABLE;
    auto minParentIndex = 0ULL;
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
        auto oldIndex = vd->parentIndex;
        if (parent != nullptr) {
            minParentLevel = parent->level;
        }
        minParentIndex = oldIndex;

        PRINT_DEBUG("Size of graph is " << n << ".");
        PRINT_DEBUG("Parent is " << parent);

        while (reachV && (parent == nullptr || vd->level <= parent->level)
               && (!levelChanged || vd->parentIndex < oldIndex)) {
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
                    vd->parentIndex = 0;
                    PRINT_DEBUG("  Maximum parent index exceeded, increasing level to "
                                << vd->level << ".")
                    PRINT_DEBUG("  Resetting index to " << vd->parentIndex << ".")
                }
            }
            if (reachV)  {
                parent = vd->getParentData();
                PRINT_DEBUG("  Trying " << parent << " as parent.");
                if (parent != nullptr
                        && (parent->level < minParentLevel
                            || (parent->level == minParentLevel
                                && vd->parentIndex < minParentIndex))) {
                    minParentLevel = parent->level;
                    minParentIndex = vd->parentIndex;
                }
            }
        }
    }
    PRINT_DEBUG("Finished search for new parent.")
    if (levelChanged) {
        PRINT_DEBUG("Level has changed, checking children in BFS tree.")
        auto updateChildren = [this,&enqueue](Arc *a) {
#ifdef COLLECT_PR_DATA
            prArcConsidered();
#endif
            if (a->isLoop()) {
              PRINT_DEBUG("  Ignoring loop.");
              return;
            }
            Vertex *head = reverseArcDirection ? a->getTail() : a->getHead();
#ifdef COLLECT_PR_DATA
            prVertexConsidered();
#endif
            auto *hd = data(head);
            if (hd->isTreeArc(a)) {
                PRINT_DEBUG("  Adding child " << hd << " to queue.");
                enqueue(hd);
            } else {
              PRINT_DEBUG("  NOT adding " << hd << " to queue: not a child of " << vd)
            }
        };
        auto checkLimit = [&limitReached](const Arc *) { return limitReached; };
        if (reverseArcDirection) {
            diGraph->mapIncomingArcsUntil(vd->getVertex(), updateChildren, checkLimit);
        } else {
            diGraph->mapOutgoingArcsUntil(vd->getVertex(), updateChildren, checkLimit);
        }
        PRINT_DEBUG("Done checking children. Limit has " << (limitReached ? "(!)" : "not")
                    << " been reached.")
        if (reachV && !limitReached) {
            if (minParentLevel == ESVertexData::UNREACHABLE) {
                PRINT_DEBUG("Vertex " << v << " is unreachable.")
                vd->setUnreachable();
                reachable.resetToDefault(v);
                reachV = false;
                levelDiff = n - oldVLevel;
            } else {
                vd->level = minParentLevel + 1;
                vd->parentIndex = minParentIndex;
            }
        }
    }

    PRINT_DEBUG("Returning level diff " << levelDiff  << " for " << vd << ".");

    assert(limitReached || vd->checkIntegrity());
#ifdef COLLECT_PR_DATA
        prVerticesConsidered(verticesConsidered);
        prArcsConsidered(arcsConsidered);
#endif

    return levelDiff;

}

template<bool reverseArcDirection>
void ESTreeML<reverseArcDirection>::restoreTree(ESVertexData *rd)
{
    auto n = diGraph->getSize();
    DiGraph::size_type affectedLimit = maxAffectedRatio < 1.0
            ? static_cast<DiGraph::size_type>(floor(maxAffectedRatio * n)) : n;
    queue.set_capacity(affectedLimit);
    timesInQueue.resetAll(n);
    timesInQueue[rd->getVertex()]++;
    queue.clear();
    queue.push_back(rd);
    if (maxReQueued == 0U) {
        maxReQueued = 1U;
    }
    PRINT_DEBUG("Initialized queue with " << rd << ".")
    bool limitReached = false;
    auto processed = 0U;

    while (!queue.empty()) {
        IF_DEBUG(printQueue(queue))
        auto vd = queue.front();
        queue.pop_front();
#ifdef COLLECT_PR_DATA
        prVertexConsidered();
        auto levels = 
#endif
        process(vd, limitReached);
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
            queue.clear();
            rerun();
            break;
#ifdef COLLECT_PR_DATA
        } else if (levels > 0U) {
            movesDown++;
            levelIncrease += levels;
            PRINT_DEBUG("total level increase " << levelIncrease);
            if (levels > maxLevelIncrease) {
                maxLevelIncrease = levels;
                PRINT_DEBUG("new max level increase " << maxLevelIncrease);
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

template<bool reverseArcDirection>
void ESTreeML<reverseArcDirection>::cleanup(bool freeSpace)
{
    if (initialized) {
        for (auto i = data.cbegin(); i != data.cend(); i++) {
            if ((*i)) {
                delete (*i);
            }
        }
    }

    queue.clear();
    if (freeSpace || !diGraph) {
        data.resetAll(0);
        reachable.resetAll(0);
        inNeighborIndices.resetAll(0);
        timesInQueue.resetAll(0);
        queue.set_capacity(0);
    } else {
        data.resetAll(diGraph->getSize());
        reachable.resetAll(diGraph->getSize());
        inNeighborIndices.resetAll(diGraph->getNumArcs(true));
        timesInQueue.resetAll(diGraph->getSize());
    }

    initialized = false ;
}

template class ESTreeML<false>;
template class ESTreeML<true>;
}
