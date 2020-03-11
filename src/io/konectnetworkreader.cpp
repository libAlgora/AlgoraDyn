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

KonectNetworkReader::KonectNetworkReader(bool antedateVertexAdditions,
                                         bool removeIsolatedEndVertices,
                                         GraphArtifact::size_type limitNumTimestamps)
    : antedateVertexAdditions(antedateVertexAdditions),
      removeIsolatedEndVertices(removeIsolatedEndVertices),
      limitNumTimestamps(limitNumTimestamps),
      strict(false)
{

}

KonectNetworkReader::~KonectNetworkReader()
{

}

bool KonectNetworkReader::provideDynamicDiGraph(DynamicDiGraph *dynGraph)
{
    if (StreamDiGraphReader::inputStream == nullptr) {
        lastError.append("No input stream.\n");
        return false;
    }
    const std::vector<char> comment = { '%', '#' };
    using namespace std;
    istream &inputStream = *(StreamDiGraphReader::inputStream);
    bool errorsOccurred = false;

    std::vector<Entry> entries;

    if (progressStream) {
        *progressStream << "Reading graph from file..." << std::flush;
    }
    for (string line; getline(inputStream, line); ) {
        istringstream iss(line);
        vector<string> tokens { istream_iterator<string>{iss},
                              istream_iterator<string>{}};

        if (tokens.empty()
                || std::find(comment.cbegin(), comment.cend(), tokens.front().front())
                        != comment.cend()) {
            continue;
        }

        if (tokens.size() < 2 || (strict && tokens.size() != 4)) {
            lastError.append(line);
            lastError.append(": Each line must contain exactly four entries.\n");
            errorsOccurred = true;
            continue;
        }

        try {
            //PRINT_DEBUG("Trying to parse tokens: " << tokens[0] << "; " << tokens[1] << "; "
            // << tokens[2] << "; " << tokens[3])
            auto tail = std::stoull(tokens[0]);
            auto head = std::stoull(tokens[1]);
            int plusMinus = tokens.size() > 2 ? std::stoi(tokens[2]) : 1;
            auto timestamp = tokens.size() > 3 ? std::stoull(tokens[3]) : 0;
            if (plusMinus > 0) {
                entries.emplace_back(tail, head, true, timestamp);
            } else if (plusMinus < 0) {
                entries.emplace_back(tail, head, false, timestamp);
            } else {
                lastError.append(line);
                lastError.append(
                            ": Don't know how to interpret a value of '0' in the third column.\n");
                errorsOccurred = true;
                continue;
            }
        } catch (const std::invalid_argument &e) {
            stringstream ss;
            ss << lastError;
            ss << "Could not parse line \"" << line << "\": " << e.what() << endl;
            lastError = ss.str();
            errorsOccurred = true;
            continue;
        }
    }

    if (entries.empty()) {
        if (progressStream) {
            *progressStream << " done. Dynamic digraph is empty!" << std::endl;
        }
        return !errorsOccurred;
    }

    if (progressStream) {
        *progressStream << " done." << std::endl;
        *progressStream << "Sorting operations by timestamp..." << std::flush;
    }

    std::stable_sort(entries.begin(), entries.end());
    auto rErrors = 0ULL;
    std::string lastRError;

    if (progressStream) {
        *progressStream << " done." << std::endl;
        *progressStream << "Creating dynamic digraph..." << std::flush;
    }

    DiGraph::size_type numTs = 1;
    unsigned long long lastTimestamp = entries.front().timestamp;

    for (const Entry &e : entries) {

        if (lastTimestamp != e.timestamp) {

            if (limitNumTimestamps > 0 && numTs >= limitNumTimestamps) {
                if (progressStream) {
                    *progressStream << " stopping after " << numTs
                                    << " timestamps at time " << lastTimestamp
                                    << "..." << std::flush;
                }
                PRINT_DEBUG("Maximum #timestamps reached at time " << e.timestamp)
                break;
            }

            numTs++;
            lastTimestamp = e.timestamp;
        }

        if (e.add) {
            PRINT_DEBUG("Adding arc " << e.tail << ", " << e.head << " at time " << e.timestamp)
            try {
                dynGraph->addArc(e.tail, e.head, e.timestamp, antedateVertexAdditions);
            } catch (const std::invalid_argument &e) {
                lastError.append(e.what());
            }
        } else {
            PRINT_DEBUG("Removing arc " << e.tail << ", " << e.head << " at time " << e.timestamp)
            try {
                dynGraph->removeArc(e.tail, e.head, e.timestamp, removeIsolatedEndVertices);
            } catch (const std::invalid_argument &ia) {
                rErrors++;
                lastRError = ia.what();
                //std::cerr << "Error at arc " << e.tail << " -> " << e.head
                // << " at time " << e.timestamp << ": " << ia.what() << std::endl;
            }
        }
    }

    if (progressStream) {
        *progressStream << " done." << std::endl;
    }
    if (rErrors > 0) {
        //std::cerr << errors << " errors occurred. Last was: " << lastError << std::endl;
        stringstream ss;
        ss << lastError;
        ss << rErrors << " remove-related errors occurred. Last was: " << lastRError << std::endl;
        lastError = ss.str();
    }

    return !errorsOccurred;
}

}
