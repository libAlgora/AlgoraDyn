#ifndef KONECTNETWORKREADER_H
#define KONECTNETWORKREADER_H

#include "io/streamdigraphreader.h"
#include <string>

namespace Algora {

class DynamicDiGraph;

class KonectNetworkReader : public StreamDiGraphReader
{
public:
    explicit KonectNetworkReader();
    virtual ~KonectNetworkReader();

    std::string getLastError() const { return lastError; }

    // DiGraphProvider interface
public:
    virtual bool provideDiGraph(DiGraph *) override { return false; }
    virtual bool provideDynamicDiGraph(DynamicDiGraph *dynGraph);

private:
    std::string lastError;
};

}

#endif // KONECTNETWORKREADER_H
