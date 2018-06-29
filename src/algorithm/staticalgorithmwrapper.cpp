/**
 * Copyright (C) 2013 - 2018 : Kathrin Hanauer
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

#include "staticalgorithmwrapper.h"

namespace Algora {

StaticAlgorithmWrapper::StaticAlgorithmWrapper(DiGraphAlgorithm *a,
                                               bool rva, bool rvr, bool raa, bool rar)
    : DynamicDiGraphAlgorithm(),
      staticAlgorithm(a),
      recomputeOnVertexAdded(rva), recomputeOnVertexRemoved(rvr),
      recomputeOnArcAdded(raa), recomputeOnArcRemoved(rar) { }

void StaticAlgorithmWrapper::onVertexAdd(Vertex *)
{
    rerunIf(recomputeOnVertexAdded);
}

void StaticAlgorithmWrapper::onVertexRemove(Vertex *)
{
    rerunIf(recomputeOnVertexRemoved);
}

void StaticAlgorithmWrapper::onArcAdd(Arc *)
{
    rerunIf(recomputeOnArcAdded);
}

void StaticAlgorithmWrapper::onArcRemove(Arc *)
{
    rerunIf(recomputeOnArcRemoved);
}

void StaticAlgorithmWrapper::rerunIf(bool rerun)
{
    if (rerun) {
       staticAlgorithm->prepare();
       staticAlgorithm->run();
    }
}

}
