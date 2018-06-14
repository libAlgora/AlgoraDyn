#ifndef LAZYBFSSSREACHALGORITHM_H
#define LAZYBFSSSREACHALGORITHM_H

#include "dynamicssreachalgorithm.h"

namespace Algora {

class LazyBFSSSReachAlgorithm : public DynamicSSReachAlgorithm
{
public:
    explicit LazyBFSSSReachAlgorithm();
    virtual ~LazyBFSSSReachAlgorithm();

    // DiGraphAlgorithm interface
public:
    virtual void run() override;
    virtual std::string getName() const noexcept override { return "Lazy BFS Single-Source Reachability Algorithm"; }
    virtual std::string getShortName() const noexcept override { return "Lazy-BFS-SSReach"; }

protected:
    virtual void onDiGraphSet() override;

    // DynamicDiGraphAlgorithm interface
protected:
    virtual void onArcAdd(Arc *) override;
    virtual void onArcRemove(Arc *) override;

    // DynamicSSReachAlgorithm interface
public:
    virtual bool query(const Vertex *t) override;

protected:
    virtual void onSourceSet() override;

private:
    struct CheshireCat;
    CheshireCat *grin;
};

}

#endif // LAZYBFSSSREACHALGORITHM_H
