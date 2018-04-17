#ifndef DYNAMICDIGRAPHALGORITHM_H
#define DYNAMICDIGRAPHALGORITHM_H

#include "algorithm/digraphalgorithm.h"

namespace Algora {

class Vertex;
class Arc;

class DynamicDiGraphAlgorithm
        : public DiGraphAlgorithm
{
public:
    explicit DynamicDiGraphAlgorithm() : DiGraphAlgorithm() {}
    virtual ~DynamicDiGraphAlgorithm() { }

protected:
    virtual void onVertexAdd(Vertex *) { }
    virtual void onVertexRemove(Vertex *) { }
    virtual void onArcAdd(Arc *) { }
    virtual void onArcRemove(Arc *) { }

    virtual void onDiGraphSet() override;
    virtual void onDiGraphUnset() override;
};

}

#endif // DYNAMICDIGRAPHALGORITHM_H
