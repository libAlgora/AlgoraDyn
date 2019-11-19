#ifndef STATICBFSAPREACHABILITYALGORITHM_H
#define STATICBFSAPREACHABILITYALGORITHM_H

#include "dynamicallpairsreachabilityalgorithm.h"

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
    virtual bool query(const Vertex *s, const Vertex *t) override;
    virtual std::vector<Arc *> queryPath(const Vertex *s, const Vertex *t) override;

private:
    bool twoWayBFS;
};

}

#endif // STATICBFSAPREACHABILITYALGORITHM_H
