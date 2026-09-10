#include <iostream>
#include <fstream>
#include <string>

#include "graph.dyn/dynamicdigraph.h"
#include "io/konectnetworkreader.h"
#include "algorithm.reachability.ss.es/simpleestree.h"

using namespace Algora;

bool readDynamicGraph(DynamicDiGraph &dyGraph, const std::string &filename);

int main(int argc, char *argv[]) {

    if (argc < 3) {
        std::cerr << "Please provide an input file, a single source vertex, and a list of target vertices, e.g.\n    "
            << argv[0]
            << " dynamic.graph 0 1 2 3 4\n";
        return 1;
    }

    DynamicDiGraph dyGraph;
    std::cout << "Reading dynamic graph " << argv[1] << "..." << std::endl;
    if (!readDynamicGraph(dyGraph, argv[1])) {
        return 1;
    }
    std::cout << "\n";

    DiGraph::size_type sourceId;
    std::vector<DiGraph::size_type> targetIds;
    try {
        sourceId = std::stoull(argv[2]);
        for (int i = 3; i < argc; i++) {
            targetIds.push_back(std::stoull(argv[i]));
        }
    } catch (const std::invalid_argument &e) {
        std::cerr << "Error parsing given vertex IDs: " << e.what() << "\n";
        return 1;
    }

    // reset to empty graph
    dyGraph.resetToBigBang();
    // get pointer to static graph that is dynamically updated
    auto graph = dyGraph.getDiGraph();

    int me = -1; // dummy identifier for observer
    // set to true if you want to know what's going on in the graph
    constexpr bool logGraphUpdates = false;

    if (logGraphUpdates) {
        graph->onVertexAdd(&me, [&](Vertex *v) {
                std::cout << "Added vertex " << v->getName()
                    << " with id " << v->getId()
                    << " at time " << dyGraph.getCurrentTime() << ".\n"; });
        graph->onVertexRemove(&me, [&](Vertex *v) {
                std::cout << "Removed vertex " << v->getName()
                    << " with id " << v->getId()
                    << " at time " << dyGraph.getCurrentTime() << ".\n"; });
        graph->onArcAdd(&me, [&](Arc *a) {
                std::cout << "Added arc " << a
                    << " with id " << a->getId()
                    << " at time " << dyGraph.getCurrentTime() << ".\n"; });
        graph->onArcRemove(&me, [&](Arc *a) {
                std::cout << "Removed arc " << a
                    << " with id " << a->getId()
                    << " at time " << dyGraph.getCurrentTime() << ".\n"; });
    }

    // apply all updates that have smallest timestamp
    dyGraph.applyNextDelta();

    // keep track of reachability from single source
    SimpleESTree<false> ses;
    ses.setGraph(graph);
    auto source = dyGraph.getCurrentVertexForId(sourceId);
    if (!source) {
        std::cerr << "Given source " << sourceId << " is not a vertex in the initial graph.\n";
        std::cerr << "Vertices are:";
        graph->mapVertices([&](Vertex *v) {
                std::cerr << v << " ";
                });
        std::cerr << "\n";
        return 1;
    }
    std::cout << "Setting source for SSR to " << source << ".\n";
    ses.setSource(source);
    if (!ses.prepare()) {
        std::cerr << "Could not prepare SES algorithm.\n";
        return 1;
    }
    ses.run();

    do {
        std::cout << "\nCurrent time: " << dyGraph.getCurrentTime()
            << "\n----------------------------------------\n";
        for (auto &vid: targetIds) {
            auto v = dyGraph.getCurrentVertexForId(vid);
            if (v) {
                auto reachable = ses.query(v);
                if (reachable) {
                    std::cout << v << " is reachable from " << source << " via " << source ;
                    auto path = ses.queryPath(v);
                    for (auto *a : path) {
                        std::cout << " -> " << a->getHead();
                    }
                    std::cout << "\n";
                } else {
                    std::cout << v << " is currently not reachable from " << source << ".\n";
                }
            }
        }
        // apply all updates with the next timestamp
        // (use dyGraph.applyNextOperation() if you just want the next operation)
    } while(dyGraph.applyNextDelta());

    if (logGraphUpdates) {
        // remove observers
        graph->removeOnVertexAdd(&me);
        graph->removeOnVertexRemove(&me);
        graph->removeOnArcAdd(&me);
        graph->removeOnArcRemove(&me);
    }

    return 0;
}

bool readDynamicGraph(DynamicDiGraph &dyGraph, const std::string &filename) {
    // create reader instance
    KonectNetworkReader reader;
    // use this version to insert all vertices at the very beginning
    // KonectNetworkReader reader(true);
    std::ifstream input(filename, std::ifstream::in);
    if (!input) {
      std::cerr << "Could not open input file " << filename << std::endl;
      return false;
    }
    reader.setInputStream(&input);
    reader.setProgressStream(&std::cout);
    if (reader.isGraphAvailable()) {
      if (reader.provideDynamicDiGraph(&dyGraph)) {
          auto errors = reader.getErrors();
          if (!errors.empty()) {
              std::cerr << "Warnings: " << errors;
          }
      } else {
          std::cerr << "Errors occurred while reading graph." << std::endl;
          auto errors = reader.getErrors();
          if (!errors.empty()) {
              std::cerr << "Errors: " << errors;
          }
      }
    } else {
      std::cerr << "No graph available. Does the file store a dynamic graph?" << std::endl;
      return false;
    }
    return true;
}
