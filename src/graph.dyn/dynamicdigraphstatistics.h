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

#ifndef DYNAMICDIGRAPHSTATISTICS_H
#define DYNAMICDIGRAPHSTATISTICS_H

namespace Algora {

class DynamicDiGraph;

class DynamicDiGraphStatistics
{
public:
    DynamicDiGraphStatistics() {}

    void analyzeDynamicDiGraph(DynamicDiGraph *dyGraph);

    unsigned long long initialGraphSize() const { return iGraphSize; }
    unsigned long long maximumGraphSize() const { return maxGraphSize; }
    unsigned long long minimumGraphSize() const { return minGraphSize; }
    unsigned long long medianGraphSize() const { return medGraphSize; }
    unsigned long long finalGraphSize() const { return fGraphSize; }
    double averageGraphSize() const { return avgGraphSize; }

    unsigned long long initialArcSize() const { return iArcSize; }
    unsigned long long maximumArcSize() const { return maxArcSize; }
    unsigned long long minimumArcSize() const { return minArcSize; }
    unsigned long long medianArcSize() const { return medArcSize; }
    unsigned long long finalArcSize() const { return fArcSize; }
    double averageArcSize() const { return avgArcSize; }

    unsigned long long minimumArcAdditions() const { return minArcAdditions; }
    unsigned long long maximumArcAdditions() const { return maxArcAdditions; }
    unsigned long long medianArcAdditions() const { return medArcAdditions; }
    double averageArcAdditions() const { return avgArcAdditions; }

    unsigned long long minimumArcRemovals() const { return minArcRemovals; }
    unsigned long long maximumArcsRemovals() const { return maxArcRemovals; }
    unsigned long long medianArcRemovals() const { return medArcRemovals; }
    double averageArcRemovals() const { return avgArcRemovals; }

    unsigned long long minimumTimeDelta() const { return minTimeDelta; }
    unsigned long long maximumTimeDelta() const { return maxTimeDelta; }
    unsigned long long medianTimeDelta() const { return medTimeDelta; }
    double averageTimeDelta() const { return avgTimeDelta; }

private:
    unsigned long long iGraphSize;
    unsigned long long maxGraphSize;
    unsigned long long minGraphSize;
    unsigned long long medGraphSize;
    unsigned long long fGraphSize;
    double avgGraphSize;

    unsigned long long iArcSize;
    unsigned long long maxArcSize;
    unsigned long long minArcSize;
    unsigned long long medArcSize;
    unsigned long long fArcSize;
    double avgArcSize;

    unsigned long long minArcAdditions;
    unsigned long long maxArcAdditions;
    unsigned long long medArcAdditions;
    double avgArcAdditions;

    unsigned long long minArcRemovals;
    unsigned long long maxArcRemovals;
    unsigned long long medArcRemovals;
    double avgArcRemovals;

    unsigned long long minTimeDelta;
    unsigned long long maxTimeDelta;
    unsigned long long medTimeDelta;
    double avgTimeDelta;
};

}

#endif // DYNAMICDIGRAPHSTATISTICS_H
