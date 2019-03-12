/**
 * Copyright (C) 2013 - 2019 : Kathrin Hanauer
 *
 * This file is part of Algora.
 *
 * Algora is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Algora is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Algora.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact information:
 *   http://algora.xaikal.org
 */

#include "konectnetworkreader.h"

#include <iterator>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>

#include "graph.dyn/dynamicdigraph.h"

//#define DEBUG_KONECTREADER

#ifdef DEBUG_KONECTREADER
#define PRINT_DEBUG(msg) std::cout << msg << std::endl;
#define IF_DEBUG(cmd) cmd;
#else
//#define PRINT_DEBUG(msg)
#define PRINT_DEBUG(msg) ((void)0);
#define IF_DEBUG(cmd)
#endif


namespace Algora {

struct Entry {
    unsigned long long tail;
    unsigned long long head;
    bool add;
    unsigned long long timestamp;

    Entry(unsigned long long t, unsigned long long h, bool a, unsigned long long m)
        : tail(t), head(h), add(a), timestamp(m) {}
};
bool operator<(const Entry &lhs, const Entry &rhs) {
    return lhs.timestamp < rhs.timestamp;
}

KonectNetworkReader::KonectNetworkReader(bool antedateVertexAdditions, bool removeIsolatedEndVertices)
    : antedateVertexAdditions(antedateVertexAdditions), removeIsolatedEndVertices(removeIsolatedEndVertices)
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

    std::vector<Entry> entries;

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
            //PRINT_DEBUG("Trying to parse tokens: " << tokens[0] << "; " << tokens[1] << "; " << tokens[2] << "; " << tokens[3])
            auto tail = std::stoull(tokens[0]);
            auto head = std::stoull(tokens[1]);
            int plusMinus = std::stoi(tokens[2]);
            auto timestamp = std::stoull(tokens[3]);
            if (plusMinus > 0) {
                entries.emplace_back(tail, head, true, timestamp);
            } else if (plusMinus < 0) {
                entries.emplace_back(tail, head, false, timestamp);
            } else {
                lastError = "Don't know how to interpret a value of '0' in third column.";
                return false;
            }
        } catch (const std::invalid_argument &e) {
            stringstream ss;
            ss << "Could not parse line \"" << line << "\": " << e.what() << endl;
            lastError = ss.str();
            return false;
        }
    }
    std::stable_sort(entries.begin(), entries.end());
    auto errors = 0ULL;
    std::string lastError;
    for (const Entry &e : entries) {
        if (e.add) {
            PRINT_DEBUG("Adding arc " << e.tail << ", " << e.head << " at time " << e.timestamp);
            try {
                dynGraph->addArc(e.tail, e.head, e.timestamp, antedateVertexAdditions);
            } catch (const std::invalid_argument &e) {
                std::cerr << e.what() << std::endl;
            }
        } else {
            PRINT_DEBUG("Removing arc " << e.tail << ", " << e.head << " at time " << e.timestamp);
            try {
                dynGraph->removeArc(e.tail, e.head, e.timestamp, removeIsolatedEndVertices);
            } catch (const std::invalid_argument &ia) {
                errors++;
                lastError = ia.what();
                //std::cerr << "Error at arc " << e.tail << " -> " << e.head << " at time " << e.timestamp << ": " << ia.what() << std::endl;
            }
        }
    }
    if (errors > 0) {
        std::cerr << errors << " errors occurred. Last was: " << lastError << std::endl;
    }

    return true;
}

}
