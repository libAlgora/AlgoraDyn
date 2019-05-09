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
    virtual ~KonectNetworkReader() override;

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
