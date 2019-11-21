#ifndef STATICBFSAPREACHABILITYALGORITHM_H
#define STATICBFSAPREACHABILITYALGORITHM_H

#include "dynamicallpairsreachabilityalgorithm.h"
#include "algorithm.basic/finddipathalgorithm.h"

namespace Algora {

class StaticBFSAPReachabilityAlgorithm : public DynamicAllPairsReachabilityAlgorithm
{
public:
    StaticBFSAPReachabilityAlgorithm(bool twoWayBFS = false);
    virtual ~StaticBFSAPReachabilityAlgorithm() override = default;

    // DiGraphAlgorithm interface
public:
    virtual void run() override;
    virtual std::string getName() const noexcept override {
        if (twoWayBFS) {
            return "Static BFS All-Pairs Reachability Algorithm (forward-backward)";
        }
        return "Static BFS All-Pairs Reachability Algorithm (forward-only)";
    }
    virtual std::string getShortName() const noexcept override {
        if (twoWayBFS) {
            return "FB-Static-BFS-APReach";
        }
        return "Static-BFS-APReach";
    }

    // DynamicAllPairsReachabilityAlgorithm interface
public:
    virtual bool query(Vertex *s, Vertex *t) override;
    virtual std::vector<Arc *> queryPath(Vertex *s, Vertex *t) override;

protected:
    virtual void onDiGraphSet() override;

private:
    bool twoWayBFS;
    DiGraph::size_type bfsStepSize;
    FindDiPathAlgorithm<FastPropertyMap> fpa;
};

}

#endif // STATICBFSAPREACHABILITYALGORITHM_H
