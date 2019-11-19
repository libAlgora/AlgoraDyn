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

#include <cassert>
#include <cmath>

#include "staticbfsssreachalgorithm.h"
#include "property/fastpropertymap.h"

#include "algorithm.basic.traversal/breadthfirstsearch.h"

namespace Algora {

StaticBFSSSReachAlgorithm::StaticBFSSSReachAlgorithm(bool twoWayBFS)
    : DynamicSingleSourceReachabilityAlgorithm(), twoWayBFS(twoWayBFS), bfsStepSize(5UL)
{
    registerEvents(false, false, false, false);
}

StaticBFSSSReachAlgorithm::~StaticBFSSSReachAlgorithm()
{

}

void StaticBFSSSReachAlgorithm::run()
{

}

void StaticBFSSSReachAlgorithm::onDiGraphSet()
{
    bfsStepSize = static_cast<DiGraph::size_type>(
                ceil(diGraph->getNumArcs(true) / diGraph->getSize()));
    if (bfsStepSize < 5) {
        bfsStepSize = 5;
    }
}

bool StaticBFSSSReachAlgorithm::query(const Vertex *t)
{
    if (t == source) {
        return true;
    } else if (diGraph->isSink(source) || diGraph->isSource(t)) {
        return false;
    }

    if (twoWayBFS) {
        return twoWayQuery(t);
    }

    BreadthFirstSearch<FastPropertyMap,false> bfs(false, false);
    bfs.setStartVertex(source);
    bool reachable = false;
#ifdef COLLECT_PR_DATA
    bfs.onArcDiscover([this](const Arc *) {
        prArcConsidered();
        return true;
    });
    bfs.onVertexDiscover([this](const Vertex *) {
        prVertexConsidered();
        return true;
    });
#endif
    bfs.setArcStopCondition([t,&reachable](const Arc *a) {
        if (a->getHead() == t) {
            reachable = true;
        }
        return reachable;
    });
    runAlgorithm(bfs, diGraph);
    return reachable;
}

std::vector<Arc *> StaticBFSSSReachAlgorithm::queryPath(const Vertex *t)
{
    std::vector<Arc*> path;
    if (t == source || diGraph->isSink(source) || diGraph->isSource(t)) {
        return path;
    }

    if (twoWayBFS) {
        return twoWayQueryPath(t);
    }

    BreadthFirstSearch<FastPropertyMap,false> bfs(false, false);
    bfs.setStartVertex(source);
    FastPropertyMap<Arc*> treeArc(nullptr);
    bool reachable = false;
#ifdef COLLECT_PR_DATA
    bfs.onArcDiscover([this](const Arc *) {
        prArcConsidered();
        return true;
    });
    bfs.onVertexDiscover([this](const Vertex *) {
        prVertexConsidered();
        return true;
    });
#endif
    bfs.onTreeArcDiscover([t,&reachable, &treeArc](const Arc *a) {
        auto head = a->getHead();
        treeArc[head] = const_cast<Arc*>(a);
        if (head == t) {
            reachable = true;
        }
        return reachable;
    });
    bfs.setArcStopCondition([&reachable](const Arc *) {
        return reachable;
    });
    runAlgorithm(bfs, diGraph);

    if (reachable) {
        while (t != source) {
            auto *a = treeArc(t);
            path.push_back(a);
            t = a->getTail();
        }
        assert(!path.empty());

        std::reverse(path.begin(), path.end());
    }

    return path;
}

bool StaticBFSSSReachAlgorithm::twoWayQuery(const Vertex *t)
{
    BreadthFirstSearch<FastPropertyMap,false> forwardBfs(false, false);
    forwardBfs.setGraph(diGraph);
    forwardBfs.setStartVertex(source);

    BreadthFirstSearch<FastPropertyMap,false> backwardBfs(false, false);
    backwardBfs.setGraph(diGraph);
    backwardBfs.reverseArcDirection(true);
    backwardBfs.setStartVertex(t);

#ifdef COLLECT_PR_DATA
    auto countArcConsidered = [this](const Arc *) {
        prArcConsidered();
        return true;
    };
    auto countVertexConsidered = [this](const Vertex *) {
        prVertexConsidered();
        return true;
    };
    forwardBfs.onArcDiscover(countArcConsidered);
    backwardBfs.onArcDiscover(countArcConsidered);
    forwardBfs.onVertexDiscover(countVertexConsidered);
    backwardBfs.onVertexDiscover(countVertexConsidered);
#endif
    bool reachable = false;
    auto forwardStop = bfsStepSize;
    auto backwardStop = bfsStepSize;

    forwardBfs.setArcStopCondition([&backwardBfs,&reachable](const Arc *a) {
        if (backwardBfs.vertexDiscovered(a->getHead())) {
            reachable = true;
        }
        return reachable;
    });
    backwardBfs.setArcStopCondition([&forwardBfs,&reachable](const Arc *a) {
        if (forwardBfs.vertexDiscovered(a->getTail())) {
            reachable = true;
        }
        return reachable;
    });

    forwardBfs.setVertexStopCondition([&forwardBfs,&forwardStop,this](const Vertex *) {
        //if (forwardBfs.getMaxLevel() >= forwardStopDepth) {
        if (forwardBfs.getMaxBfsNumber() >= forwardStop) {
            forwardStop += bfsStepSize;
            return true;
        }
        return false;
    });

    backwardBfs.setVertexStopCondition([&backwardBfs,&backwardStop,this](const Vertex *) {
        if (backwardBfs.getMaxBfsNumber() >= backwardStop) {
            backwardStop += bfsStepSize;
            return true;
        }
        return false;
    });

    forwardBfs.prepare();
    backwardBfs.prepare();

    forwardBfs.run();
    backwardBfs.run();

    while (!reachable && !forwardBfs.isExhausted() && !backwardBfs.isExhausted()) {
        forwardBfs.resume();
        backwardBfs.resume();
    }

    return reachable;
}

std::vector<Arc *> StaticBFSSSReachAlgorithm::twoWayQueryPath(const Vertex *t)
{
    BreadthFirstSearch<FastPropertyMap,false> forwardBfs(false, false);
    forwardBfs.setGraph(diGraph);
    forwardBfs.setStartVertex(source);

    BreadthFirstSearch<FastPropertyMap,false> backwardBfs(false, false);
    backwardBfs.setGraph(diGraph);
    backwardBfs.reverseArcDirection(true);
    backwardBfs.setStartVertex(t);

#ifdef COLLECT_PR_DATA
    auto countArcConsidered = [this](const Arc *) {
        prArcConsidered();
        return true;
    };
    auto countVertexConsidered = [this](const Vertex *) {
        prVertexConsidered();
        return true;
    };
    forwardBfs.onArcDiscover(countArcConsidered);
    backwardBfs.onArcDiscover(countArcConsidered);
    forwardBfs.onVertexDiscover(countVertexConsidered);
    backwardBfs.onVertexDiscover(countVertexConsidered);
#endif
    auto forwardStop = bfsStepSize;
    auto backwardStop = bfsStepSize;
    FastPropertyMap<Arc*> treeArc(nullptr);
    Arc *fbLink = nullptr;

    forwardBfs.setArcStopCondition([fbLink](const Arc *) {
        return fbLink != nullptr;
    });
    backwardBfs.setArcStopCondition([fbLink](const Arc *) {
        return fbLink != nullptr;
    });

    forwardBfs.setVertexStopCondition([&forwardBfs,&forwardStop,this](const Vertex *) {
        if (forwardBfs.getMaxBfsNumber() >= forwardStop) {
            forwardStop+= bfsStepSize;
            return true;
        }
        return false;
    });

    backwardBfs.setVertexStopCondition([&backwardBfs,&backwardStop,this](const Vertex *) {
        if (backwardBfs.getMaxBfsNumber() >= backwardStop) {
            backwardStop+= bfsStepSize;
            return true;
        }
        return false;
    });

    forwardBfs.onTreeArcDiscover([&backwardBfs,&treeArc,&fbLink](const Arc *a) {
        if (fbLink) {
            return;
        }
        auto head = a->getHead();
        if (backwardBfs.vertexDiscovered(head)) {
            fbLink = const_cast<Arc*>(a);
        } else {
            treeArc[head] = const_cast<Arc*>(a);
        }
    });
    backwardBfs.onTreeArcDiscover([&forwardBfs,&treeArc,&fbLink](const Arc *a) {
        if (fbLink) {
            return;
        }
        auto tail = a->getTail();
        if (forwardBfs.vertexDiscovered(tail)) {
            fbLink = const_cast<Arc*>(a);
        } else {
            treeArc[tail] = const_cast<Arc*>(a);
        }
    });

    forwardBfs.prepare();
    backwardBfs.prepare();

    forwardBfs.run();
    backwardBfs.run();

    while (!fbLink && !forwardBfs.isExhausted() && !backwardBfs.isExhausted()) {
        forwardBfs.resume();
        backwardBfs.resume();
    }

    std::vector<Arc*> path;
    if (fbLink) {
        auto v = fbLink->getTail();
        while (v != source) {
            auto *a = treeArc(v);
            path.push_back(a);
            v = a->getTail();
        }
        std::reverse(path.begin(), path.end());
        path.push_back(fbLink);
        v = fbLink->getHead();
        while (v != t) {
            auto *a = treeArc(v);
            path.push_back(a);
            v = a->getHead();
        }

        assert(!path.empty());
    }

    return path;
}

}
