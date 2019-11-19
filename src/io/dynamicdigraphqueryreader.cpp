#include "dynamicdigraphqueryreader.h"

#include <map>
#include <sstream>
#include <iterator>
#include <iostream>

namespace Algora {

std::vector<DynamicDiGraphQueryProvider::VertexQueryList>
DynamicDiGraphQueryReader::provideVertexQueries(DynamicDiGraph *dyGraph)
{
    std::vector<DynamicDiGraphQueryProvider::VertexQueryList> allQueries;
    if (!checkInputStream()) {
        return allQueries;
    }

    const char comment = '%';
    std::map<DynamicDiGraph::DynamicTime, VertexQueryList> map;

    for (std::string line; std::getline(*inputStream, line); ) {
        std::istringstream iss(line);
        std::vector<std::string> tokens { std::istream_iterator<std::string>{iss},
                                          std::istream_iterator<std::string>{}};

        if (tokens.empty() || tokens.front().front() == comment) {
            continue;
        }

        try {
            auto timestamp = std::stoull(tokens[0]);

            for (auto i = 1U; i < tokens.size(); i++) {
                auto id = std::stoull(tokens[i]);
                map[timestamp].push_back(id);
            }
        } catch (const std::invalid_argument &e) {
            std::stringstream ss;
            ss << lastError;
            ss << "ERROR: Could not parse line \"" << line << "\": " << e.what() << std::endl;
            lastError = ss.str();
        }
    }

    dyGraph->resetToBigBang();
    auto mapIt = map.begin();

    for (const auto &graphTime : dyGraph->getTimestamps()) {
        auto mapTime = mapIt->first;
        while (mapTime < graphTime && mapIt != map.end()) {
            std::stringstream ss;
            ss << lastError;
            ss << "WARN: Queries for time " << mapTime << " are ignored." << std::endl;
            lastError = ss.str();
            mapIt++;
            mapTime = mapIt->first;
        }
        if (mapTime == graphTime) {
            allQueries.push_back(std::move(mapIt->second));
            mapIt++;
            mapTime = mapIt->first;
        } else {
            allQueries.emplace_back();
        }
    }

    // Special treatment for NOOPs at end
    if (dyGraph->getSizeOfFinalDelta()
            == dyGraph->countNoops(dyGraph->getMaxTime(), dyGraph->getMaxTime())) {
        allQueries.pop_back();
    }

    return allQueries;
}

bool DynamicDiGraphQueryReader::checkInputStream() const
{
    return inputStream != nullptr && inputStream->good() && inputStream->rdbuf()->in_avail() > 0;
}


}
