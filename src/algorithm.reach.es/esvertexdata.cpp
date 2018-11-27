#include "esvertexdata.h"

#include "graph.incidencelist/incidencelistvertex.h"
#include "graph/arc.h"
#include "algorithm.reach/dynamicssreachalgorithm.h"

#include <cassert>
//#define DEBUG_ESVD

#ifdef DEBUG_ESVD
#include <iostream>
#define PRINT_DEBUG(msg) std::cerr << msg << std::endl;
#define IF_DEBUG(cmd) cmd;
#else
#define PRINT_DEBUG(msg)
#define IF_DEBUG(cmd)
#endif

namespace Algora {

ESVertexData::ESVertexData(FastPropertyMap<unsigned int> *inNeighborIndices, Vertex *v,
                           ESVertexData *p, const Arc *treeArc, unsigned int l)
    : parentIndex(0U), level(l), vertex(v), inNeighborIndices(inNeighborIndices)
{
    if (p != nullptr) {
        (*inNeighborIndices)[treeArc] = 1U;
        inNeighbors.push_back(p);
        level = p->level + 1;
    }
}

void ESVertexData::reset(ESVertexData *p, const Arc *treeArc, unsigned int l)
{
    inNeighbors.clear();
    recycledIndices.clear();
    parentIndex = 0U;
    level = l;
    if (p != nullptr) {
        (*inNeighborIndices)[treeArc] = 1U;
        inNeighbors.push_back(p);
        level = p->level + 1;
    }
}

void ESVertexData::setUnreachable() { parentIndex = 0U; level = UNREACHABLE; }

bool ESVertexData::isReachable() const { return level != UNREACHABLE; }

void ESVertexData::addInNeighbor(ESVertexData *in, const Arc *a)
{
    assert((*inNeighborIndices)[a] == 0U);
    // try to find an empty place
    if (recycledIndices.empty()) {
        (*inNeighborIndices)[a] = inNeighbors.size() + 1U;
        inNeighbors.push_back(in);
        PRINT_DEBUG("Added vertex at the end (real index " << ((*inNeighborIndices)[a] - 1U) << ")");
    } else {
        auto i = recycledIndices.back();
        recycledIndices.pop_back();
        assert(inNeighbors[i] == nullptr);
        inNeighbors[i] = in;
        (*inNeighborIndices)[a] = i + 1U;
        PRINT_DEBUG("Inserted vertex at index " << i);
    }
}

unsigned int ESVertexData::reparent(ESVertexData *in, const Arc *a)
{
    auto inLevel = in->level;
    if (inLevel >= level) {
        return 0U;
    } else {
        auto index = (*inNeighborIndices)[a] - 1U;
        if (inLevel + 1 < level) {
            parentIndex = index;
            auto diff = level - (inLevel + 1U);
            level = inLevel + 1U;
            return diff;
        } else if (index < parentIndex) {
            parentIndex = index;
        }
        return 0U;
    }
}

void ESVertexData::findAndRemoveInNeighbor(ESVertexData *in, const Arc *a)
{
    auto index = (*inNeighborIndices)[a] - 1U;
    assert(inNeighbors[index] == in);
    inNeighbors[index] = nullptr;
    inNeighborIndices->resetToDefault(a);
    recycledIndices.push_back(index);
}

bool ESVertexData::isParent(ESVertexData *p)
{
    if (parentIndex >= inNeighbors.size() || !isReachable()) {
        return false;
    }
    return inNeighbors[parentIndex] == p;

}

ESVertexData *ESVertexData::getParentData() const
{
    if (parentIndex >= inNeighbors.size() || !isReachable()) {
        return nullptr;
    }
    return inNeighbors[parentIndex];
}

Vertex *ESVertexData::getParent() const
{
    auto p = getParentData();
    if (p == nullptr) {
        return nullptr;
    }
    return p->vertex;
}

bool ESVertexData::checkIntegrity() const
{
    return (isReachable() && (level == 0 || getParentData()->level + 1 == level))
            || (!isReachable() && getParentData() == nullptr);

}

std::ostream &operator<<(std::ostream &os, const ESVertexData *vd) {
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

}
