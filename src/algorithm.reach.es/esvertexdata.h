#ifndef ESVERTEXDATA_H
#define ESVERTEXDATA_H

#include <vector>
#include <climits>
#include <iostream>
#include <property/fastpropertymap.h>

namespace Algora {

class DynamicDiGraphAlgorithm;
class Vertex;
class Arc;

class ESVertexData
{
    friend std::ostream& operator<<(std::ostream &os, const ESVertexData *vd);
public:
    static const unsigned int UNREACHABLE = UINT_MAX;

    ESVertexData(FastPropertyMap<unsigned int> *inNeighborIndices,
            Vertex *v, ESVertexData *p = nullptr, const Arc *treeArc = nullptr, unsigned int l = UINT_MAX);

    void reset(ESVertexData *p = nullptr, const Arc *treeArc = nullptr, unsigned int l = UINT_MAX);

    void setUnreachable();
    bool isReachable() const;
    unsigned int getLevel() const {
        return isReachable() ? level : UNREACHABLE;
    }
    Vertex *getVertex() const { return vertex; }

    void addInNeighbor(ESVertexData *in, const Arc *a);
    unsigned int reparent(ESVertexData *in, const Arc *a);
    void findAndRemoveInNeighbor(ESVertexData *in, const Arc *a);

    bool isParent(ESVertexData *p);
    ESVertexData *getParentData() const;
    Vertex *getParent() const;

    bool checkIntegrity() const;

    std::vector<ESVertexData*> inNeighbors;
    unsigned int parentIndex;
    unsigned int level;

private:
    Vertex *vertex;
    FastPropertyMap<unsigned int> *inNeighborIndices;
    std::vector<unsigned int> recycledIndices;
};

std::ostream& operator<<(std::ostream& os, const ESVertexData *vd);

struct ES_Priority { int operator()(const ESVertexData *vd) { return vd->getLevel(); }};

}

#endif // ESVERTEXDATA_H
