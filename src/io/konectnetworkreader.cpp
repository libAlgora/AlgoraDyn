#include "konectnetworkreader.h"

#include <iterator>
#include <sstream>
#include <vector>
#include <string>

#include "graph.dyn/dynamicdigraph.h"

namespace Algora {

KonectNetworkReader::KonectNetworkReader()
{

}

KonectNetworkReader::~KonectNetworkReader()
{

}

bool KonectNetworkReader::provideDynamicDiGraph(DynamicDiGraph *dynGraph)
{
    lastError.clear();
    if (StreamDiGraphReader::inputStream == 0) {
        lastError = "No input stream.";
        return false;
    }
    const char comment = '%';
    using namespace std;
    istream &inputStream = *(StreamDiGraphReader::inputStream);

    for (string line; getline(inputStream, line); ) {
        istringstream iss(line);
        vector<string> tokens { istream_iterator<string>{iss},
                              istream_iterator<string>{}};

        if (tokens.empty() || tokens.front().front() == comment) {
            continue;
        }
        if (tokens.size() != 4) {
            lastError = "Each line must contain exactly four entries.";
            return false;
        }
        try {
            int tail = std::stoi(tokens[0]);
            int head = std::stoi(tokens[1]);
            int plusMinus = std::stoi(tokens[2]);
            unsigned long timestamp = std::stoul(tokens[3]);
            if (plusMinus > 0) {
                dynGraph->addArc(tail, head, timestamp);
            } else if (plusMinus < 0) {
                dynGraph->removeArc(tail, head, timestamp);
            } else {
                lastError = "Don't know how to interpret a value of '0' in third column.";
                return false;
            }
        } catch (const std::invalid_argument &) {
            lastError = "Could not parse value.";
            return false;
        }

    }

    return true;
}

}
