#ifndef DYNAMICDIGRAPHQUERYREADER_H
#define DYNAMICDIGRAPHQUERYREADER_H

#include "pipe/dynamicdigraphqueryprovider.h"

#include <istream>

namespace Algora {

class DynamicDiGraphQueryReader : public DynamicDiGraphQueryProvider
{
public:
    DynamicDiGraphQueryReader(std::istream *input = nullptr) : inputStream(input), lastError("") { }
    void setInputStream(std::istream *input) { inputStream = input; }

    // DynamicDiGraphQueryProvider interface
public:
    virtual std::vector<VertexQueryList> provideVertexQueries(DynamicDiGraph *dyGraph) override;
    virtual std::string getName() const noexcept override { return "Dynamic DiGraph Query Reader"; }

private:
    std::istream *inputStream;
    std::string lastError;

    bool checkInputStream() const;
};

}

#endif // DYNAMICDIGRAPHQUERYREADER_H
