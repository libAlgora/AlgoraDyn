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

#ifndef STATICALGORITHMWRAPPER_H
#define STATICALGORITHMWRAPPER_H

#include "dynamicdigraphalgorithm.h"

namespace Algora {

class StaticAlgorithmWrapper
        : public DynamicDiGraphAlgorithm
{
public:
    explicit StaticAlgorithmWrapper(DiGraphAlgorithm *a, bool rva = true, bool rvr = true,
                                    bool raa = true, bool rar = true);
    virtual ~StaticAlgorithmWrapper() { delete staticAlgorithm; }

    // DiGraphAlgorithm interface
public:
    virtual bool prepare() override { return staticAlgorithm->prepare(); }
    virtual void run() override { staticAlgorithm->run(); }
    virtual std::string getName() const noexcept override { return "Dynamized " + staticAlgorithm->getName(); }
    virtual std::string getShortName() const noexcept override { return "dyz-" + staticAlgorithm->getShortName(); }

protected:
    virtual void onDiGraphSet() override {
        DynamicDiGraphAlgorithm::onDiGraphSet();
        staticAlgorithm->setGraph(diGraph);
    }

    // DynamicDiGraphAlgorithm interface
protected:
    virtual void onVertexAdd(Vertex *) override;
    virtual void onVertexRemove(Vertex *) override;
    virtual void onArcAdd(Arc *) override;
    virtual void onArcRemove(Arc *) override;

private:
    DiGraphAlgorithm *staticAlgorithm;
    bool recomputeOnVertexAdded;
    bool recomputeOnVertexRemoved;
    bool recomputeOnArcAdded;
    bool recomputeOnArcRemoved;

    void rerunIf(bool rerun);
};

}

#endif // STATICALGORITHMWRAPPER_H
