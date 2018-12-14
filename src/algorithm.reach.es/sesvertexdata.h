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
