#ifndef STATICBFSSSREACHALGORITHM_H
#define STATICBFSSSREACHALGORITHM_H

#include "dynamicssreachalgorithm.h"

namespace Algora {

class StaticBFSSSReachAlgorithm : public DynamicSSReachAlgorithm
{
public:
    explicit StaticBFSSSReachAlgorithm();
    virtual ~StaticBFSSSReachAlgorithm();

    // DiGraphAlgorithm interface
public:
    virtual void run() override;
    virtual std::string getName() const noexcept override { return "Static BFS Single-Source Reachability Algorithm"; }
    virtual std::string getShortName() const noexcept override { return "Static-BFS-SSReach"; }

    // DynamicSSReachAlgorithm interface
public:
    virtual bool query(const Vertex *t) override;
};

}

#endif // STATICBFSSSREACHALGORITHM_H
