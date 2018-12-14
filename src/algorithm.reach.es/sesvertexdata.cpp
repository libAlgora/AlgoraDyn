#include "sesvertexdata.h"

namespace Algora {

std::ostream &operator<<(std::ostream &os, const SESVertexData *vd)
{
    if (vd == nullptr) {
        os << " null ";
        return os;
    }

    os << vd->vertex << ": ";
    //os << "parent: [" << vd->parent << "] ; level: " << vd->level;
    os << "parent: [";
    if (vd->parent) {
      os << vd->parent->vertex << ", level: " << vd->parent->level;
    } else {
        os << "null";
    }
    os << "] ; level: " << vd->level;
    return os;
}

}
