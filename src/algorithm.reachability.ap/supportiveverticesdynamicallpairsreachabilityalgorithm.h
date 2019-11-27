// license
#ifndef SUPPORTIVEVERTICESDYNAMICALLPAIRSREACHABILITYALGORITHM_H
#define SUPPORTIVEVERTICESDYNAMICALLPAIRSREACHABILITYALGORITHM_H

#include "dynamicallpairsreachabilityalgorithm.h"
#include "property/fastpropertymap.h"

namespace Algora {

template<typename DynamicSSRAlgorithm>
class SupportiveVerticesDynamicAllPairsReachabilityAlgorithm
        : public DynamicAllPairsReachabilityAlgorithm
{
public:
    explicit SupportiveVerticesDynamicAllPairsReachabilityAlgorithm(double supportSizeRatio);
    virtual ~SupportiveVerticesDynamicAllPairsReachabilityAlgorithm();

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

    // DiGraphAlgorithm interface
protected:
    virtual void onDiGraphUnset() override;

    // DynamicAllPairsReachabilityAlgorithm interface
public:
    virtual bool query(Vertex *s, Vertex *t) override;
    virtual std::vector<Arc *> queryPath(Vertex *, Vertex *) override;

private:
    FastPropertyMap<DynamicSSRAlgorithm*> supportiveVertexToSSRAlgorithm;
    std::vector<DynamicSSRAlgorithm*> supportiveSSRAlgorithms;
    bool initialized;
    double supportSizeRatio;

    void reset();
    void createAndInitAlgorithm(Vertex *v);
};

}

#include "supportiveverticesdynamicallpairsreachabilityalgorithm.cpp"

#endif // SUPPORTIVEVERTICESDYNAMICALLPAIRSREACHABILITYALGORITHM_H
