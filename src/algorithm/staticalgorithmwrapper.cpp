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
