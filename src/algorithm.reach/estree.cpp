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

#include "estree.h"

#include <vector>
#include <climits>
#include <cassert>

#include "graph/vertex.h"
#include "algorithm.basic.traversal/breadthfirstsearch.h"
#include "algorithm/digraphalgorithmexception.h"
#include "datastructure/bucketqueue.h"
#include "property/fastpropertymap.h"

//#define DEBUG_ESTREE

#ifdef DEBUG_ESTREE
#include <iostream>
#define PRINT_DEBUG(msg) std::cerr << msg << std::endl;
#define IF_DEBUG(cmd) cmd;
#else
#define PRINT_DEBUG(msg)
#define IF_DEBUG(cmd)
#endif

namespace Algora {

struct ESTree::VertexData {
    static const unsigned int UNREACHABLE = UINT_MAX;
    static unsigned int graphSize;
    static double CLEANUP_AFTER;
    static unsigned int cleanups;

    Vertex *vertex;
    std::vector<VertexData*> inNeighbors;
    unsigned int parentIndex;
    unsigned int level;
    unsigned int inNeighborsLost = 0U;

    VertexData(Vertex *v, VertexData *p = nullptr, unsigned int l = UINT_MAX)
        : vertex(v), parentIndex(0), level(l), inNeighborsLost(0U) {
        if (p != nullptr) {
            inNeighbors.push_back(p);
            level = p->level + 1;
        }
    }

    void reset(VertexData *p = nullptr, unsigned int l = UINT_MAX) {
        inNeighbors.clear();
        inNeighborsLost = 0U;
        parentIndex = 0U;
        level = l;
        if (p != nullptr) {
            inNeighbors.push_back(p);
            level = p->level + 1;
        }
    }

    void setUnreachable() {
        parentIndex = 0;
        level = UNREACHABLE;
        // compact list
        cleanupInNeighbors();
    }

    void cleanupInNeighbors() {
        cleanups++;
        assert(parentIndex < inNeighbors.size() || parentIndex == 0);
        if (inNeighborsLost < inNeighbors.size()) {
            auto nSize = 0U;
            auto lost = 0U;
            for (auto i = 0U; i < inNeighbors.size(); i++) {
                assert(nSize <= i);
                auto n = inNeighbors[i];
                if (n) {
                    if (i != nSize) {
                        inNeighbors[nSize] = n;
                        inNeighbors[i] = nullptr;
                        if (i == parentIndex) {
                            parentIndex = nSize;
                        }
                    }
                    nSize++;
                } else {
                    lost++;
                }
            }
            assert(lost == inNeighborsLost);
            assert(lost + nSize == inNeighbors.size());
            inNeighbors.erase(inNeighbors.cbegin() + nSize, inNeighbors.cend());
            if (parentIndex >= nSize) {
                parentIndex = 0;
            }
        } else {
            inNeighbors.clear();
            parentIndex = 0U;
        }
        inNeighborsLost = 0U;
    }

    bool isReachable() const {
        return level != UNREACHABLE;
    }

    unsigned int priority() const {
        return isReachable() ? level : graphSize + 1U;
    }

    void findAndRemoveInNeighbor(VertexData *in) {
        bool found = false;
        for (auto i = inNeighbors.begin(); i != inNeighbors.end() && !found; i++) {
            if ((*i) != nullptr && (*i) == in) {
               *i = nullptr;
               PRINT_DEBUG("Removed " << in->vertex << " in N- of " << vertex);
               found = true;
               inNeighborsLost++;
            }
        }
        if (inNeighborsLost > 4 && inNeighborsLost > inNeighbors.size() * CLEANUP_AFTER) {
            cleanupInNeighbors();
        }

        assert(found);
    }

    bool isParent(VertexData *p) {
        if (parentIndex >= inNeighbors.size() || !isReachable()) {
            return false;
        }
        return inNeighbors[parentIndex] == p;
    }

    VertexData *getParentData() const {
        if (parentIndex >= inNeighbors.size() || !isReachable()) {
            return nullptr;
        }
        return inNeighbors[parentIndex];
    }

    Vertex *getParent() const {
        auto p = getParentData();
        if (p == nullptr) {
            return nullptr;
        }
        return p->vertex;
    }

    bool checkIntegrity() const {
        return (isReachable() && (level == 0 || getParentData()->level + 1 == level))
                || (!isReachable() && getParentData() == nullptr);
    }
};

unsigned int ESTree::VertexData::graphSize = 0U;
double ESTree::VertexData::CLEANUP_AFTER = 1.0;
unsigned int ESTree::VertexData::cleanups = 0U;

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

struct ESNode_Priority { int operator()(const ESTree::VertexData *vd) { return vd->priority(); }};
typedef BucketQueue<ESTree::VertexData*, ESNode_Priority> PriorityQueue;

#ifdef DEBUG_ESTREE
void printQueue(PriorityQueue q) {
    std::cerr << "PriorityQueue: ";
    while(!q.empty()) {
        std::cerr << q.bot()->vertex << "[" << q.bot()->level << "]" << ", ";
        q.popBot();
    }
    std::cerr << std::endl;
}
#endif

unsigned int process(DiGraph *graph, ESTree::VertexData *vd, PriorityQueue &queue,
                     const FastPropertyMap<ESTree::VertexData*> &data,
                     FastPropertyMap<bool> &reachable,
                     FastPropertyMap<bool> &inQueue,
                     FastPropertyMap<unsigned int> &timesInQueue, unsigned int requeueLimit, bool &limitReached, unsigned int &maxRequeued);

ESTree::ESTree(double cleanupAfter, unsigned int requeueLimit, double maxAffectedRatio)
    : DynamicSSReachAlgorithm(), root(nullptr),
      initialized(false), requeueLimit(requeueLimit),
      maxAffectedRatio(maxAffectedRatio), cleanupAfter(cleanupAfter),
      movesDown(0U), movesUp(0U),
      levelIncrease(0U), levelDecrease(0U),
      maxLevelIncrease(0U), maxLevelDecrease(0U),
      decUnreachableHead(0U), decNonTreeArc(0U),
      incUnreachableTail(0U), incNonTreeArc(0U),
      reruns(0U), maxReQueued(0U)
{
    data.setDefaultValue(nullptr);
    reachable.setDefaultValue(false);
    VertexData::CLEANUP_AFTER = cleanupAfter;
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
        Vertex *t = a->getTail();
        Vertex *h = a->getHead();
        if (data[h] == nullptr) {
            data[h] = new VertexData(h, data(t));
        } else {
            data[h]->reset(data(t));
        }
        reachable[h] = true;
        PRINT_DEBUG( "(" << t << ", " << h << ")" << " is a tree arc.")
   });
   bfs.onNonTreeArcDiscover([&](Arc *a) {
        if (a->isLoop() || a->getHead() == source) {
            return;
        }
        Vertex *t = a->getTail();
        Vertex *h = a->getHead();
        VertexData *td = data(t);
        VertexData *hd = data(h);
        hd->inNeighbors.push_back(td);
        PRINT_DEBUG( "(" << td->vertex << ", " << hd->vertex << ")" << " is a non-tree arc.")
   });
   runAlgorithm(bfs, diGraph);

   diGraph->mapArcs([&](Arc *a) {
       if (a->isLoop() || a->getHead() == source) {
           return;
       }
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

   diGraph->mapVertices([&](Vertex *v) {
       if (data(v) == nullptr) {
           data[v] = new VertexData(v);
           PRINT_DEBUG( v << " is unreachable.")
       }
   });

   VertexData::graphSize = diGraph->getSize();
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

std::string ESTree::getProfilingInfo() const
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
    ss << "requeue limit: " << requeueLimit << std::endl;
    ss << "maximum #requeuings: " << maxReQueued << std::endl;
    ss << "maximum ratio of affected vertices: " << maxAffectedRatio << std::endl;
    ss << "#reruns: " << reruns << std::endl;
    ss << "cleanup after: " << cleanupAfter << std::endl;
    ss << "#cleanups: " << VertexData::cleanups << std::endl;
    return ss.str();
}

DynamicSSReachAlgorithm::Profile ESTree::getProfile() const
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
    profile.push_back(std::make_pair(std::string("requeue_limit"), requeueLimit));
    profile.push_back(std::make_pair(std::string("max_affected"), maxAffectedRatio));
    profile.push_back(std::make_pair(std::string("max_requeued"), maxReQueued));
    profile.push_back(std::make_pair(std::string("rerun"), reruns));
    profile.push_back(std::make_pair(std::string("cleanup_after"), cleanupAfter));
    profile.push_back(std::make_pair(std::string("cleanups"), VertexData::cleanups));
    return profile;
}

void ESTree::onDiGraphSet()
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
    VertexData::cleanups = 0U;
    data.resetAll(diGraph->getSize());
    reachable.resetAll(diGraph->getSize());
}

void ESTree::onDiGraphUnset()
{
    cleanup();
    DynamicSSReachAlgorithm::onDiGraphUnset();
}

void ESTree::onVertexAdd(Vertex *v)
{
    data[v] = new VertexData(v);
    VertexData::graphSize++;
}

void ESTree::onArcAdd(Arc *a)
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

    auto n = diGraph->getSize();
    //update...
    if (hd->level <= td->level + 1) {
        // arc does not change anything
        incNonTreeArc++;
        return;
    } else {
        movesUp++;
        if (!hd->isReachable()) {
            levelDecrease += (n - (td->level + 1));
        } else {
            levelDecrease += (hd->level - (td->level + 1));
        }
        hd->level = td->level + 1;
        reachable[head] = true;
    }

    std::vector<VertexData*> verticesToProcess;
    verticesToProcess.push_back(hd);


    BreadthFirstSearch<FastPropertyMap> bfs(false);
    bfs.setStartVertex(head);
    bfs.onArcDiscover([&](const Arc *a) {
        PRINT_DEBUG( "Discovering arc (" << a->getTail() << ", " << a->getHead() << ")...");
        if (a->isLoop()) {
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
            ahd->level = newLevel;
            ahd->parentIndex = 0;
            reachable[ah] = true;
            verticesToProcess.push_back(ahd);
            PRINT_DEBUG( "(" << at << ", " << ah << ")" << " is a new tree arc.");
            return true;
        } else if (atd->level + 1 == ahd->level && !ahd->isParent(atd)) {
            ahd->parentIndex = 0;
            verticesToProcess.push_back(ahd);
            PRINT_DEBUG( "(" << at << ", " << ah << ")" << " may become a new tree arc.");
        } else {
            PRINT_DEBUG( "(" << at << ", " << ah << ")" << " is a non-tree arc.")
        }
        return false;
    });
    runAlgorithm(bfs, diGraph);

    auto oldLevelInc = levelIncrease;
    restoreTree(verticesToProcess);
    assert(oldLevelInc == levelIncrease);

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

void ESTree::onVertexRemove(Vertex *v)
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

void ESTree::onArcRemove(Arc *a)
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

    std::stringstream ss;
    dumpTree(ss);

    VertexData *hd = data(head);
    if (hd == nullptr) {
        PRINT_DEBUG("Head of arc is unreachable (and never was). Nothing to do.")
        throw std::logic_error("Should not happen");
        return;
    }

    VertexData *td = data(tail);
    bool isParent = hd->isParent(td);
    hd->findAndRemoveInNeighbor(td);

    if (!hd->isReachable()) {
        PRINT_DEBUG("Head of arc is already unreachable. Nothing to do.")
        decUnreachableHead++;
        return;
    }

    if (hd->level <= td->level || !isParent) {
        PRINT_DEBUG("Arc is not a tree arc. Nothing to do.");
        decNonTreeArc++;
    } else {
        restoreTree({ hd });
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
            os << (*i) << std::endl;
        }
    }
}

void ESTree::dumpTree(std::ostream &os)
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

bool ESTree::checkTree()
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
       auto bfsLevel = levels[v] < 0 ? ESTree::VertexData::UNREACHABLE : levels[v];
       if (data[v]->level != bfsLevel) {
           std::cerr << "Level mismatch for vertex " << data[v]
                        << ": expected level " << bfsLevel << std::endl;
           ok = false;
       }
       if (!data[v]->checkIntegrity()) {
           std::cerr << "Integrity check failed for vertex " << data[v] << std::endl;
           ok = false;
       }
   });
   return ok;
}

void ESTree::rerun()
{
    reruns++;
    diGraph->mapVertices([&](Vertex *v) {
        data[v]->reset();
    });
    initialized = false;
    run();
}

void ESTree::restoreTree(const std::vector<ESTree::VertexData *> vds)
{
    PriorityQueue queue;
    FastPropertyMap<bool> inQueue(false, "", diGraph->getSize());
    FastPropertyMap<unsigned int> timesInQueue(0U, "", diGraph->getSize());
    for (auto vd : vds) {
        if (!inQueue[vd->vertex]) {
            queue.push(vd);
            inQueue[vd->vertex] = true;
            timesInQueue[vd->vertex]++;
        }
    }
    PRINT_DEBUG("Initialized queue with " << vds.size() << " vertices.");
    bool limitReached = false;
		auto affected = 0U;
		auto maxAffected = maxAffectedRatio * VertexData::graphSize;

    while (!queue.empty()) {
        IF_DEBUG(printQueue(queue))
        auto vd = queue.bot();
        queue.popBot();
        inQueue[vd->vertex] = false;
        unsigned int levels = process(diGraph, vd, queue, data, reachable, inQueue, timesInQueue, requeueLimit,
                                      limitReached, maxReQueued);
        if (limitReached || affected > maxAffected) {
            rerun();
            break;
        } else if (levels > 0U) {
						affected++;
            movesDown++;
            levelIncrease += levels;
            PRINT_DEBUG("total level increase " << levelIncrease);
            if (levels > maxLevelIncrease) {
                maxLevelIncrease = levels;
                PRINT_DEBUG("new max level increase " << maxLevelIncrease);
            }
        }
    }
}

void ESTree::cleanup()
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

unsigned int process(DiGraph *graph, ESTree::VertexData *vd, PriorityQueue &queue,
                     const FastPropertyMap<ESTree::VertexData *> &data,
                     FastPropertyMap<bool> &reachable,
                     FastPropertyMap<bool> &inQueue,
                     FastPropertyMap<unsigned int> &timesInQueue, unsigned int requeueLimit, bool &limitReached, unsigned int &maxRequeued) {

    if (vd->level == 0UL) {
        PRINT_DEBUG("No need to process source vertex " << vd << ".");
        return 0U;
    }

    PRINT_DEBUG("Processing vertex " << vd << ".");

    if (!vd->isReachable()) {
        PRINT_DEBUG("vertex is already unreachable.");
        return 0U;
    }

    Vertex *v = vd->vertex;
    bool reachV = true;
    bool levelChanged = false;
    auto n = graph->getSize();
    unsigned int oldVLevel = vd->level;
    unsigned int levelDiff = 0U;

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

        bool inNeighborFound = parent != nullptr;
        unsigned int minimumParentLevel = parent != nullptr ? parent->level : UINT_MAX;

        PRINT_DEBUG("Parent is " << parent);

        while (reachV && (parent == nullptr || vd->level <= parent->level)) {
            vd->parentIndex++;
            PRINT_DEBUG("  Advancing parent index to " << vd->parentIndex << ".")

            if (vd->parentIndex >= vd->inNeighbors.size()) {
                if ((vd->level + 1 >= graph->getSize()) || (levelChanged && (!inNeighborFound || minimumParentLevel == UINT_MAX))) {
                    PRINT_DEBUG("    Vertex " << v << " is unreachable (source: " << (levelChanged ? (inNeighborFound ? "no" : "yes" ) : "?") << ").")
                    vd->setUnreachable();
                    reachable.resetToDefault(v);
                    reachV = false;
                    levelChanged = true;
                    levelDiff = n - oldVLevel;
                } else {
                    if (levelChanged) {
                        assert(oldVLevel < minimumParentLevel + 1);
                        levelDiff = minimumParentLevel + 1 - oldVLevel;
                        vd->level = minimumParentLevel + 1;
                    } else {
                        vd->level++;
                        levelDiff++;
                        levelChanged = true;
                    }
                    PRINT_DEBUG("  Maximum parent index exceeded, increasing level to " << vd->level << ".")
                    vd->parentIndex = 0;
                }
            }
            if (reachV)  {
                parent = vd->getParentData();
                inNeighborFound |= parent != nullptr;
                if (parent && parent->level < minimumParentLevel) {
                    minimumParentLevel = parent->level;
                    PRINT_DEBUG("  Minimum parent level is now " << minimumParentLevel << ".")
                }
                PRINT_DEBUG("  Trying " << parent << " as parent.")
            }
        }

    }
    if (levelChanged) {
        graph->mapOutgoingArcsUntil(vd->vertex, [&](Arc *a) {
            if (a->isLoop()) {
              PRINT_DEBUG("    Ignoring loop.");
              return;
            }
            Vertex *head = a->getHead();
            auto *hd = data(head);
            if (hd->isParent(vd) && !inQueue[head]) {
              if (timesInQueue[head] < requeueLimit) {
                PRINT_DEBUG("    Adding child " << hd << " to queue...")
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
            } else {
              PRINT_DEBUG("    NOT adding " << hd << " to queue: not a child of " << vd)
            }
        }, [&limitReached](const Arc *) { return limitReached; });
    }

    PRINT_DEBUG("Returning level diff " << levelDiff  << " for " << vd << ".");

    assert(vd->checkIntegrity());

    return levelDiff;
}

}
