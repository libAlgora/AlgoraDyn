#ifndef DYNAMICDIGRAPH_H
#define DYNAMICDIGRAPH_H

namespace Algora {

class DiGraph;

class DynamicDiGraph
{
public:
    DynamicDiGraph();
    ~DynamicDiGraph();

    DiGraph *getDiGraph() const;
    unsigned int getCurrentTime() const;
    unsigned int getMaxTime() const;

    void addVertex(unsigned int timestamp);
    void removeVertex(unsigned int vertexId, unsigned int timestamp);
    void addArc(unsigned int tailId, unsigned int headId, unsigned int timestamp);
    void removeArc(unsigned int tailId, unsigned int headId, unsigned int timestamp);

    void resetToBigBang();
    bool applyNextOperation();
    bool applyNextDelta();

private:
    class CheshireCat;
    CheshireCat *grin;
};

}

#endif // DYNAMICDIGRAPH_H
