#ifndef CACHINGBFSSSREACHALGORITHM_H
#define CACHINGBFSSSREACHALGORITHM_H

#include "algorithm.reach/dynamicssreachalgorithm.h"
#include "algorithm.basic/breadthfirstsearch.h"
#include "property/propertymap.h"

namespace Algora {

class CachingBFSSSReachAlgorithm : public DynamicSSReachAlgorithm
{
public:
    explicit CachingBFSSSReachAlgorithm();
    virtual ~CachingBFSSSReachAlgorithm();

    // DiGraphAlgorithm interface
public:
    virtual void run() override;
    virtual std::string getName() const noexcept override { return "Caching BFS Single-Source Reachability Algorithm";  }
    virtual std::string getShortName() const noexcept override { return "CachingBFS-SSReach"; }

    // DynamicSSReachAlgorithm interface
    virtual bool query(const Vertex *t) override;

protected:
    // DiGraphAlgorithm interface
    virtual void onDiGraphSet() override;
    virtual void onDiGraphUnset() override;

    // DynamicDiGraphAlgorithm interface
    virtual void onArcAdd(Arc *) override;
    virtual void onArcRemove(Arc *) override;

    // DynamicSSReachAlgorithm interface
protected:
    virtual void onSourceSet() override;

private:
    BreadthFirstSearch bfs;
    PropertyMap<int> levels;
    bool initialized;
};

}

#endif // CACHINGBFSSSREACHALGORITHM_H
