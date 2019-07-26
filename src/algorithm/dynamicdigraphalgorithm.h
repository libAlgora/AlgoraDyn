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

#ifndef DYNAMICDIGRAPHALGORITHM_H
#define DYNAMICDIGRAPHALGORITHM_H

#include "algorithm/digraphalgorithm.h"

namespace Algora {

class Vertex;
class Arc;

class DynamicDiGraphAlgorithm
        : public DiGraphAlgorithm
{
public:
    explicit DynamicDiGraphAlgorithm() : DiGraphAlgorithm(), autoUpdate(true), registered(false),
        registerOnVertexAdd(true), registerOnVertexRemove(true), registerOnArcAdd(true), registerOnArcRemove(true)
    {}
    virtual ~DynamicDiGraphAlgorithm() override { deregister(); }

    void setAutoUpdate(bool au) {
        this->autoUpdate = au;
    }

    bool doesAutoUpdate() const {
        return this->autoUpdate;
    }

    virtual void onVertexAdd(Vertex *) { }
    virtual void onVertexRemove(Vertex *) { }
    virtual void onArcAdd(Arc *) { }
    virtual void onArcRemove(Arc *) { }

protected:
    virtual void onDiGraphSet() override;
    virtual void onDiGraphUnset() override;

    void registerEvents(bool vertexAdd, bool vertexRemove, bool arcAdd, bool arcRemove) {
        registerOnVertexAdd = vertexAdd;
        registerOnVertexRemove = vertexRemove;
        registerOnArcAdd = arcAdd;
        registerOnArcRemove = arcRemove;
    }

private:
    bool autoUpdate;
    bool registered;

    void deregister();

    bool registerOnVertexAdd;
    bool registerOnVertexRemove;
    bool registerOnArcAdd;
    bool registerOnArcRemove;
};

}

#endif // DYNAMICDIGRAPHALGORITHM_H
