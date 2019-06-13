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

#include "dynamicdigraphalgorithm.h"

#include "graph/digraph.h"
#include <functional>

namespace Algora {

void DynamicDiGraphAlgorithm::onDiGraphSet()
{
    DiGraphAlgorithm::onDiGraphSet();

    if (autoUpdate) {
        using namespace std::placeholders;  // for _1, _2, _3...
        //auto ova = std::bind(&DynamicDiGraphAlgorithm::onVertexAdd, this, _1);
        diGraph->onVertexAdd(this, std::bind(&DynamicDiGraphAlgorithm::onVertexAdd, this, _1));
        diGraph->onVertexRemove(this, std::bind(&DynamicDiGraphAlgorithm::onVertexRemove, this, _1));
        diGraph->onArcAdd(this, std::bind(&DynamicDiGraphAlgorithm::onArcAdd, this, _1));
        diGraph->onArcRemove(this, std::bind(&DynamicDiGraphAlgorithm::onArcRemove, this, _1));
        registered = true;
    }
}

void DynamicDiGraphAlgorithm::onDiGraphUnset()
{
    deregister();
    DiGraphAlgorithm::onDiGraphUnset();
}

void DynamicDiGraphAlgorithm::deregister()
{
    if (diGraph && registered) {
        diGraph->removeOnVertexAdd(this);
        diGraph->removeOnVertexRemove(this);
        diGraph->removeOnArcAdd(this);
        diGraph->removeOnArcRemove(this);
        registered = false;
    }
}

}
