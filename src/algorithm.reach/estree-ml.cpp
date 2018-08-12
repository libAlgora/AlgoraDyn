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

#include "estree-ml.h"

#include <vector>
#include <climits>
#include <cassert>

#include "graph/vertex.h"
#include "algorithm.basic.traversal/breadthfirstsearch.h"
#include "algorithm/digraphalgorithmexception.h"
#include "datastructure/bucketqueue.h"

//#define DEBUG_ESTREEML

#ifdef DEBUG_ESTREEML
#include <iostream>
#define PRINT_DEBUG(msg) std::cerr << msg << std::endl;
#define IF_DEBUG(cmd) cmd;
#else
#define PRINT_DEBUG(msg)
#define IF_DEBUG(cmd)
#endif

namespace Algora {

struct ESTreeML::VertexData {
    static const unsigned int UNREACHABLE = UINT_MAX;
    static unsigned int graphSize;

    Vertex *vertex;
    std::vector<VertexData*> inNeighbors;
    std::vector<unsigned int> inNeighborsTimes;
    unsigned int parentIndex;
    unsigned int level;
    unsigned int inNeighborsLost = 0U;
    // offset 1
    FastPropertyMap<unsigned int> inNeighborIndices;

    VertexData(Vertex *v, VertexData *p = nullptr, unsigned int l = UINT_MAX)
        : vertex(v), parentIndex(0), level(l), inNeighborsLost(0U) {
        inNeighborIndices.setDefaultValue(0U);
        inNeighborIndices.resetAll(graphSize);
        if (p != nullptr) {
            inNeighborIndices[p->vertex] = 1U;
            inNeighbors.push_back(p);
            inNeighborsTimes.push_back(1U);
            level = p->level + 1;
        }
    }

    void reset(VertexData *p = nullptr, unsigned int l = UINT_MAX) {
        for (auto in : inNeighbors) {
            if (in != nullptr) {
                inNeighborIndices.resetToDefault(in->vertex);
            }
        }
        inNeighbors.clear();
        inNeighborsTimes.clear();
        inNeighborsLost = 0U;
        parentIndex = 0U;
        level = l;
        if (p != nullptr) {
            inNeighborIndices[p->vertex] = 1U;
            inNeighbors.push_back(p);
            inNeighborsTimes.push_back(1U);
            level = p->level + 1;
        }
    }

    void setUnreachable() {
        parentIndex = 0;
        level = UNREACHABLE;
    }

    bool isReachable() const {
        return level != UNREACHABLE;
    }

    unsigned int priority() const {
        return isReachable() ? level : graphSize + 1U;
    }

    bool addInNeighbor(VertexData *in) {
        auto index = inNeighborIndices[in->vertex];
        PRINT_DEBUG("Index of " << in->vertex << " is " << index);
        if (index == 0U) {
            // try to find an empty place
            bool inserted = false;
            for (int i = inNeighbors.size() - 1; i >= 0; i--) {
                if (inNeighbors[i] == nullptr) {
                    assert(inNeighborsTimes[i] == 0U);
                    inNeighbors[i] = in;
                    inNeighborsTimes[i] = 1U;
                    inNeighborIndices[in->vertex] = i + 1U;
                    inserted = true;
                    PRINT_DEBUG("Inserted vertex at index " << i);
                    assert(inNeighborsLost > 0U);
                    inNeighborsLost--;
                    break;
                }
            }
            if (!inserted) {
                inNeighborIndices[in->vertex] = inNeighbors.size() + 1U;
                inNeighbors.push_back(in);
                inNeighborsTimes.push_back(1U);
                PRINT_DEBUG("Added vertex at the end (real index " << (inNeighborIndices[in->vertex] - 1U) << ")");
            }
            return false;
        } else {
            inNeighborsTimes[index - 1U]++;
            PRINT_DEBUG("Increased counter to " << inNeighborsTimes[index - 1U] );
            return true;
        }
    }

    unsigned int reparent(VertexData *in) {
        auto inLevel = in->level;
        if (inLevel >= level) {
            return 0U;
        } else {
            auto index = inNeighborIndices[in->vertex] - 1U;
            if (inLevel + 1 < level) {
                parentIndex = index;
                auto diff = (level == UNREACHABLE ? graphSize : level) - (inLevel + 1U);
                level = inLevel + 1U;
                return diff;
            } else if (index < parentIndex) {
                parentIndex = index;
            }
            return 0U;
        }
    }

    bool findAndRemoveInNeighbor(VertexData *in) {
        auto index = inNeighborIndices[in->vertex] - 1U;
        assert(inNeighborsTimes[index] > 0U);
        inNeighborsTimes[index]--;
        if (inNeighborsTimes[index] > 0U) {
            return true;
        }
        inNeighbors[index] = nullptr;
        inNeighborsTimes[index] = 0U;
        inNeighborIndices.resetToDefault(in->vertex);
        inNeighborsLost++;
        return false;
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

unsigned int ESTreeML::VertexData::graphSize = 0U;

std::ostream& operator<<(std::ostream& os, const ESTreeML::VertexData *vd) {
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

struct ESNode_Priority { int operator()(const ESTreeML::VertexData *vd) { return vd->priority(); }};
typedef BucketQueue<ESTreeML::VertexData*, ESNode_Priority> PriorityQueue;

#ifdef DEBUG_ESTREEML
void printQueue(PriorityQueue q) {
    std::cerr << "PriorityQueue: ";
    while(!q.empty()) {
        std::cerr << q.bot()->vertex << "[" << q.bot()->level << "]" << ", ";
        q.popBot();
    }
    std::cerr << std::endl;
}
#endif

unsigned int process(DiGraph *graph, ESTreeML::VertexData *vd, PriorityQueue &queue,
                     const FastPropertyMap<ESTreeML::VertexData*> &data,
                     FastPropertyMap<bool> &reachable,
                     FastPropertyMap<bool> &inQueue,
                     FastPropertyMap<unsigned int> &timesInQueue, unsigned int requeueLimit, bool &limitReached, unsigned int &maxRequeued);

ESTreeML::ESTreeML(unsigned int requeueLimit, double maxAffectedRatio)
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
    reachable.setDefaultValue(false);
}

ESTreeML::~ESTreeML()
{
    cleanup();
}

void ESTreeML::run()
{
    if (initialized) {
        return;
    }

   PRINT_DEBUG("Initializing ESTreeML...")

   VertexData::graphSize = diGraph->getSize();
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
        PRINT_DEBUG( "(" << t << ", " << h << ")" << " is a tree arc.")
        if (data[h] == nullptr) {
            data[h] = new VertexData(h, data(t));
        } else {
            data[h]->reset(data(t));
        }
        reachable[h] = true;
   });
   bfs.onNonTreeArcDiscover([&](Arc *a) {
        if (a->isLoop() || a->getHead() == source) {
            return;
        }
        Vertex *t = a->getTail();
        Vertex *h = a->getHead();
        VertexData *td = data(t);
        VertexData *hd = data(h);
        PRINT_DEBUG( "(" << td->vertex << ", " << hd->vertex << ")" << " is a non-tree arc.")
        hd->addInNeighbor(td);
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
            PRINT_DEBUG( "(" << td->vertex << ", " << hd->vertex << ")" << " is an unvisited non-tree arc.")
            hd->addInNeighbor(td);
       }
   });

   diGraph->mapVertices([&](Vertex *v) {
       if (data(v) == nullptr) {
           data[v] = new VertexData(v);
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

std::string ESTreeML::getProfilingInfo() const
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
    ss << "total affected vertices: " << totalAffected << std::endl;
    ss << "maximum number of affected vertices: " << maxAffected << std::endl;
    ss << "#reruns: " << reruns << std::endl;
    return ss.str();
}

DynamicSSReachAlgorithm::Profile ESTreeML::getProfile() const
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
    profile.push_back(std::make_pair(std::string("total_affected"), totalAffected));
    profile.push_back(std::make_pair(std::string("max_affected"), maxAffected));
    profile.push_back(std::make_pair(std::string("rerun"), reruns));
    return profile;
}

void ESTreeML::onDiGraphSet()
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

void ESTreeML::onDiGraphUnset()
{
    cleanup();
    DynamicSSReachAlgorithm::onDiGraphUnset();
}

void ESTreeML::onVertexAdd(Vertex *v)
{
    data[v] = new VertexData(v);
    VertexData::graphSize++;
}

void ESTreeML::onArcAdd(Arc *a)
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
    if (hd->addInNeighbor(td)) {
        PRINT_DEBUG("Parallel arc is already there.")
        return;
    }

    if (!td->isReachable()) {
        PRINT_DEBUG("Tail is unreachable.")
        incUnreachableTail++;
        return;
    }

    //update...
    auto diff = hd->reparent(td);
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
        if (a->isLoop()) {
            return false;
        }
        Vertex *at = a->getTail();
        Vertex *ah = a->getHead();
        VertexData *atd = data(at);
        VertexData *ahd = data(ah);

        auto diff = ahd->reparent(atd);
        if (diff > 0U) {
            PRINT_DEBUG("Is a new tree arc.");
            movesUp++;
            reachable[ah] = true;
            if (diff > maxLevelDecrease) {
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

void ESTreeML::onVertexRemove(Vertex *v)
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

void ESTreeML::onArcRemove(Arc *a)
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

    VertexData *hd = data(head);
    if (hd == nullptr) {
        PRINT_DEBUG("Head of arc is unreachable (and never was). Nothing to do.")
        throw std::logic_error("Should not happen");
        return;
    }

    VertexData *td = data(tail);
    bool isParent = hd->isParent(td);
    if (hd->findAndRemoveInNeighbor(td)) {
        return;
    }

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

void ESTreeML::onSourceSet()
{
    cleanup();
}

bool ESTreeML::query(const Vertex *t)
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

void ESTreeML::dumpData(std::ostream &os)
{
    if (!initialized) {
        os << "uninitialized" << std::endl;
    }  else {
        for (auto i = data.cbegin(); i != data.cend(); i++) {
            os << (*i) << std::endl;
        }
    }
}

void ESTreeML::dumpTree(std::ostream &os)
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

bool ESTreeML::checkTree()
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
       auto bfsLevel = levels[v] < 0 ? ESTreeML::VertexData::UNREACHABLE : levels[v];
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

void ESTreeML::rerun()
{
    reruns++;
    diGraph->mapVertices([&](Vertex *v) {
        data[v]->reset();
    });
    initialized = false;
    run();
}

void ESTreeML::restoreTree(ESTreeML::VertexData *vd)
{
    PriorityQueue queue;
    FastPropertyMap<bool> inQueue(false, "", diGraph->getSize());
    FastPropertyMap<unsigned int> timesInQueue(0U, "", diGraph->getSize());
    queue.push(vd);
    inQueue[vd->vertex] = true;
    timesInQueue[vd->vertex]++;
    PRINT_DEBUG("Initialized queue with " << vds.size() << " vertices.");
    bool limitReached = false;
    auto affected = 0U;
    auto affectedLimit = maxAffectedRatio * VertexData::graphSize;

    while (!queue.empty()) {
        IF_DEBUG(printQueue(queue))
        auto vd = queue.bot();
        queue.popBot();
        inQueue[vd->vertex] = false;
        unsigned int levels = process(diGraph, vd, queue, data, reachable, inQueue, timesInQueue, requeueLimit,
                                      limitReached, maxReQueued);
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

void ESTreeML::cleanup()
{
    for (auto i = data.cbegin(); i != data.cend(); i++) {
        if ((*i)) {
            delete (*i);
        }
    }

    data.resetAll();
    reachable.resetAll();

    initialized = false ;

}

unsigned int process(DiGraph *graph, ESTreeML::VertexData *vd, PriorityQueue &queue,
                     const FastPropertyMap<ESTreeML::VertexData *> &data,
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

    auto enqueue = [&](ESTreeML::VertexData *vd) {
        auto vertex = vd->vertex;
        if (timesInQueue[vertex] < requeueLimit) {
            PRINT_DEBUG("    Adding " << vd << " to queue...");
            timesInQueue[vertex]++;
            if (timesInQueue[vertex] > maxRequeued) {
                maxRequeued = timesInQueue[vertex];
            }
            queue.push(vd);
            //queue.push_back(vd);
            inQueue[vertex] = true;
        } else {
            timesInQueue[vertex]++;
            limitReached = true;
            PRINT_DEBUG("Limit reached for vertex " << vertex << ".");
        }
    };

    auto minParentLevel = UINT_MAX;
    auto minParentIndex = 0U;
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
            vd->parentIndex++;
            PRINT_DEBUG("  Advancing parent index to " << vd->parentIndex << ".")

            if (vd->parentIndex >= vd->inNeighbors.size()) {
                if (vd->level + 1 >= graph->getSize()) {
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
            if (reachV)  {
                parent = vd->getParentData();
                PRINT_DEBUG("  Trying " << parent << " as parent.");
                if (parent != nullptr
                        && (parent->level < minParentLevel
                            || (parent->level == minParentLevel && vd->parentIndex < minParentIndex))) {
                    minParentLevel = parent->level;
                    minParentIndex = vd->parentIndex;
                }
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
                enqueue(hd);
            } else {
              PRINT_DEBUG("    NOT adding " << hd << " to queue: not a child of " << vd)
            }
        }, [&limitReached](const Arc *) { return limitReached; });
        if (reachV && !limitReached) {
            if (minParentLevel == UINT_MAX) {
                PRINT_DEBUG("    Vertex " << v << " is unreachable.")
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

    assert(vd->checkIntegrity());

    return levelDiff;
}

}
