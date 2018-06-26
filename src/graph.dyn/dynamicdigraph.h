#ifndef DYNAMICDIGRAPH_H
#define DYNAMICDIGRAPH_H

namespace Algora {

class DiGraph;

class DynamicDiGraph
{
public:
    explicit DynamicDiGraph();
    ~DynamicDiGraph();

    DiGraph *getDiGraph() const;
    unsigned int getCurrentTime() const;
    unsigned int getMaxTime() const;
    unsigned int getNumberOfDeltas() const;
    unsigned int getCurrentGraphSize() const;
    unsigned int getCurrentArcSize() const;

    unsigned int addVertex(unsigned int timestamp);
    void addVertex(unsigned int vertexId, unsigned int timestamp);
    void removeVertex(unsigned int vertexId, unsigned int timestamp);
    void addArc(unsigned int tailId, unsigned int headId, unsigned int timestamp, bool antedateVertexAdditions = false);
    void removeArc(unsigned int tailId, unsigned int headId, unsigned int timestamp, bool removeIsolatedEnds = false);
    bool hasArc(unsigned int tailId, unsigned int headId);
    void clear();

    void resetToBigBang();
    bool applyNextOperation();
    bool applyNextDelta();

    unsigned int countVertexAdditions(unsigned int timeFrom, unsigned int timeUntil) const;
    unsigned int countVertexRemovals(unsigned int timeFrom, unsigned int timeUntil) const;
    unsigned int countArcAdditions(unsigned int timeFrom, unsigned int timeUntil) const;
    unsigned int countArcRemovals(unsigned int timeFrom, unsigned int timeUntil) const;

    void squashTimes(unsigned int timeFrom, unsigned int timeUntil);

private:
    class CheshireCat;
    CheshireCat *grin;
};

}

#endif // DYNAMICDIGRAPH_H
