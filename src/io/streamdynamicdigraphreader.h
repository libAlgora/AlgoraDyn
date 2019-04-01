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

#ifndef STREAMDYNAMICDIGRAPHREADER_H
#define STREAMDYNAMICDIGRAPHREADER_H

#include "pipe/dynamicdigraphprovider.h"
#include <istream>

namespace Algora {

class StreamDynamicDiGraphReader : public DynamicDiGraphProvider
{
public:
    explicit StreamDynamicDiGraphReader(std::istream *input = nullptr) : inputStream(input) { }
    virtual ~StreamDynamicDiGraphReader() override;

    void setInputStream(std::istream *input) { inputStream = input; }

    // DynamicDiGraphProvider interface
public:
    virtual bool isGraphAvailable() override {
        return inputStream != nullptr && inputStream->good() && inputStream->rdbuf()->in_avail() > 0;
    }

protected:
    std::istream *inputStream;
};

StreamDynamicDiGraphReader::~StreamDynamicDiGraphReader() { }

}

#endif // STREAMDYNAMICDIGRAPHREADER_H
