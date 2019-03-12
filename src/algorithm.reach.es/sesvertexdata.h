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

#ifndef SESVERTEXDATA_H
#define SESVERTEXDATA_H

#include <climits>
#include <iostream>

namespace Algora {

class Vertex;

class SESVertexData
{
    friend std::ostream& operator<<(std::ostream &os, const SESVertexData *vd);

public:
    static constexpr unsigned long long UNREACHABLE = ULLONG_MAX;

    SESVertexData(Vertex *v, SESVertexData *p = nullptr, unsigned long long l = UNREACHABLE)
        : vertex(v), parent(p), level(l) {
        if (p != nullptr) {
            level = p->level + 1;
        }
    }

    void reset(SESVertexData *p = nullptr, unsigned long long l = UNREACHABLE) {
        parent = p;
        level = l;
        if (p != nullptr) {
            level = p->level + 1;
        }
    }

    unsigned long long getLevel() const {
        return level;
    }

    Vertex *getVertex() const { return vertex; }
    SESVertexData *getParentData() const { return parent; }

    void setUnreachable() {
        parent = nullptr;
        level = UNREACHABLE;
    }

    bool isReachable() const {
        return level != UNREACHABLE;
    }

    bool isParent(SESVertexData *p) {
        return p == parent;
    }

    bool hasValidParent() const {
        return parent != nullptr && parent->level + 1 == level;
    }

    Vertex *getParent() const {
        if (parent == nullptr) {
            return nullptr;
        }
        return parent->vertex;
    }

//private:
    Vertex *vertex;
    SESVertexData *parent;
    unsigned long long level;
};

std::ostream& operator<<(std::ostream& os, const SESVertexData *vd);

struct SES_Priority { unsigned long long operator()(const SESVertexData *vd) { return vd->getLevel(); }};

}

#endif // SESVERTEXDATA_H
