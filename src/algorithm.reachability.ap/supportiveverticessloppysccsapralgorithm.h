// license
#ifndef SUPPORTIVEVERTICESSLOPPYSCCSAPRALGORITHM_H
#define SUPPORTIVEVERTICESSLOPPYSCCSAPRALGORITHM_H

#include "supportiveverticesdynamicallpairsreachabilityalgorithm.h"

namespace Algora {

template<typename DynamicSingleSourceAlgorithm, typename DynamicSingleSinkAlgorithm,
         bool reAdjust = false>
class SupportiveVerticesSloppySCCsAPRAlgorithm
        : public SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<
                                                            DynamicSingleSourceAlgorithm,
                                                            DynamicSingleSinkAlgorithm,
                                                            reAdjust>
{
public:
    typedef
        SupportiveVerticesDynamicAllPairsReachabilityAlgorithm<
                                                            DynamicSingleSourceAlgorithm,
                                                            DynamicSingleSinkAlgorithm,
                                                            reAdjust> Super;

    explicit SupportiveVerticesSloppySCCsAPRAlgorithm(double supportSize, unsigned long adjustAfter);
    explicit SupportiveVerticesSloppySCCsAPRAlgorithm(double supportSize,
            unsigned long adjustAfter,
            const typename Super::SingleSourceParameterSet &ssourceParams,
            const typename Super::SingleSinkParameterSet &ssinkParams);

    explicit SupportiveVerticesSloppySCCsAPRAlgorithm(
            const typename Super::ParameterSet &params,
            const typename Super::SingleSourceParameterSet &ssourceParams,
            const typename Super::SingleSinkParameterSet &ssinkParams);

        // DiGraphAlgorithm interface
public:
        virtual void run() override;
        virtual std::string getName() const noexcept override;
        virtual std::string getShortName() const noexcept override;

        // DynamicDiGraphAlgorithm interface
public:
        virtual void onVertexAdd(Vertex *v) override;
        virtual void onVertexRemove(Vertex *v) override;
        virtual void onArcAdd(Arc *a) override;
        virtual void onArcRemove(Arc *a) override;

private:
        FastPropertyMap<Vertex*> vertexToSCCRepresentative;
        DiGraph::size_type minSccSize = 5;
        void checkSCCs();
        void createSupportVertex(Vertex *v);
};


}

#include "supportiveverticessloppysccsapralgorithm.cpp"

#endif // SUPPORTIVEVERTICESSLOPPYSCCSAPRALGORITHM_H
