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
#include "graph.dyn/dynamicweighteddigraph.h"

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

template<typename weight_type>
struct Entry {
    unsigned long long tail;
    unsigned long long head;
    weight_type weight;
    unsigned long long timestamp;

    Entry(unsigned long long t, unsigned long long h, const weight_type &w, unsigned long long m)
        : tail(t), head(h), weight(w), timestamp(m) { }
};

template<typename weight_type>
bool operator<(const Entry<weight_type> &lhs, const Entry<weight_type> &rhs) {
    return lhs.timestamp < rhs.timestamp;
}

template<typename weight_type, typename weight_parser>
bool parseEntries(std::istream &inputStream,
                  weight_parser parser,
                  bool strict,
                  std::vector<Entry<weight_type>> &entries,
                  std::string &lastError, std::ostream *progressStream) {

    using namespace  std;
    bool errorsOccurred = false;
    const vector<char> comment = { '%', '#' };

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
            bool parsedOk = false;
            weight_type weight = parser(tokens.size() > 2 ? tokens[2] : "",
                                          parsedOk);
            if (!parsedOk) {
                lastError.append(line);
                lastError.append(": Couldn't parse entry in third column.\n");
                errorsOccurred = true;
                continue;
            }
            auto timestamp = tokens.size() > 3 ? std::stoull(tokens[3]) : 0;
            entries.emplace_back(tail, head, weight, timestamp);
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
            *progressStream << " done. Instance is empty!" << std::endl;
        }
        return !errorsOccurred;
    }

    if (progressStream) {
        *progressStream << " done." << std::endl;
        *progressStream << "Sorting operations by timestamp..." << std::flush;
    }

    std::stable_sort(entries.begin(), entries.end());

    if (progressStream) {
        *progressStream << " done." << std::endl;
    }

    return !errorsOccurred;
}


KonectNetworkReader::KonectNetworkReader(bool antedateVertexAdditions,
                                         bool removeIsolatedEndVertices,
                                         GraphArtifact::size_type limitNumTimestamps)
    : antedateVertexAdditions(antedateVertexAdditions),
      removeIsolatedEndVertices(removeIsolatedEndVertices),
      limitNumTimestamps(limitNumTimestamps),
      strict(false),
      arcLifetime(0U),
      relativeWeights(false),
      removeNonPositiveArcs(true)

{  }

bool KonectNetworkReader::provideDynamicDiGraph(DynamicDiGraph *dynGraph)
{
    if (StreamDiGraphReader::inputStream == nullptr) {
        lastError.append("No input stream.\n");
        return false;
    }

    auto weight_to_bool = [](const std::string &s, bool &ok) {
        if (s.empty()) {
            ok = true;
            return true;
        }
        try {
            ok = true;
            int plusMinus = std::stoi(s);
            if (plusMinus == 0) {
                ok = false;
                return false;
            }
            return plusMinus > 0;
        } catch (std::invalid_argument &e) {
            ok = false;
        }
        return false;
    };

    typedef Entry<bool> entry_type;
    std::vector<entry_type> entries;
    bool errorsOccurred = !parseEntries<>(*(StreamDiGraphReader::inputStream),
                                                    weight_to_bool,
                                                    strict, entries, lastError, progressStream);

    auto rErrors = 0ULL;
    std::string lastRError;

    if (progressStream) {
        *progressStream << "Creating dynamic digraph..." << std::flush;
    }

    DiGraph::size_type numTs = 1;
    unsigned long long lastTimestamp = entries.front().timestamp;

    for (const auto &e : entries) {

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

        if (e.weight) {
            if (arcLifetime > 0) {
                PRINT_DEBUG("Adding arc " << e.tail << ", " << e.head << " with lifetime " << arcLifetime << " at time " << e.timestamp)
                        try {
                    dynGraph->addArcAndRemoveIn(e.tail, e.head, e.timestamp, arcLifetime, antedateVertexAdditions);
                } catch (const std::invalid_argument &e) {
                    lastError.append(e.what());
                }
            } else {
                PRINT_DEBUG("Adding arc " << e.tail << ", " << e.head << " at time " << e.timestamp)
                        try {
                    dynGraph->addArc(e.tail, e.head, e.timestamp, antedateVertexAdditions);
                } catch (const std::invalid_argument &e) {
                    lastError.append(e.what());
                }
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
        std::stringstream ss;
        ss << lastError;
        ss << rErrors << " remove-related errors occurred. Last was: " << lastRError << std::endl;
        lastError = ss.str();
    }

    return !errorsOccurred;
}

bool KonectNetworkReader::provideDynamicWeightedDiGraph(DynamicWeightedDiGraph<unsigned long> *dywGraph)
{
    if (StreamDiGraphReader::inputStream == nullptr) {
        lastError.append("No input stream.\n");
        return false;
    }

    auto parse_weight = [](const std::string &s, bool &ok) {
        if (s.empty()) {
            ok = true;
            return 1L;
        }
        try {
            ok = true;
            return std::stol(s);
        } catch (std::invalid_argument &e) {
            ok = false;
        }
        return 0L;
    };

    typedef Entry<long long> entry_type;
    std::vector<entry_type> entries;
    bool errorsOccurred = !parseEntries<>(*(StreamDiGraphReader::inputStream),
                                                    parse_weight,
                                                    strict, entries, lastError, progressStream);

    auto rErrors = 0ULL;
    std::string lastRError;

    if (progressStream) {
        *progressStream << "Creating dynamic weighted digraph..." << std::flush;
    }

    DiGraph::size_type numTs = 1;
    unsigned long long lastTimestamp = entries.front().timestamp;

    for (const auto &e : entries) {

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

        if (relativeWeights) {
            PRINT_DEBUG("Adding/updating arc " << e.tail << ", " << e.head << " by relative weight " << e.weight << " at time " << e.timestamp);
            unsigned long aWeight = e.weight >= 0 ? e.weight : -e.weight;
            dywGraph->addWeightedArcOrChangeWeightRelative(e.tail, e.head, aWeight, e.weight >= 0, removeNonPositiveArcs, e.timestamp);
        } else {
            if (removeNonPositiveArcs && e.weight <= 0) {
                PRINT_DEBUG("Removing arc " << e.tail << ", " << e.head << " due to weight " << e.weight << " at time " << e.timestamp);
                dywGraph->removeWeightedArc(e.tail, e.head, e.timestamp);
            } else {
                PRINT_DEBUG("Adding/updating arc " << e.tail << ", " << e.head << " with weight " << e.weight << " at time " << e.timestamp);
                dywGraph->addWeightedArcOrChangeWeight(e.tail, e.head, e.weight, e.timestamp);
            }
        }
    }

    if (progressStream) {
        *progressStream << " done." << std::endl;
    }
    if (rErrors > 0) {
        //std::cerr << errors << " errors occurred. Last was: " << lastError << std::endl;
        std::stringstream ss;
        ss << lastError;
        ss << rErrors << " remove-related errors occurred. Last was: " << lastRError << std::endl;
        lastError = ss.str();
    }

    return !errorsOccurred;
}

}
