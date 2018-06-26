#ifndef KONECTNETWORKREADER_H
#define KONECTNETWORKREADER_H

#include "io/streamdigraphreader.h"
#include <string>

namespace Algora {

class DynamicDiGraph;

class KonectNetworkReader : public StreamDiGraphReader
{
public:
    explicit KonectNetworkReader(bool antedateVertexAdditions = false, bool removeIsolatedEndVertices = false);
    virtual ~KonectNetworkReader();

    std::string getLastError() const { return lastError; }

    // DiGraphProvider interface
public:
    virtual bool provideDiGraph(DiGraph *) override { return false; }
    virtual bool provideDynamicDiGraph(DynamicDiGraph *dynGraph);

private:
    std::string lastError;
    bool antedateVertexAdditions;
    bool removeIsolatedEndVertices;
};

}

#endif // KONECTNETWORKREADER_H
