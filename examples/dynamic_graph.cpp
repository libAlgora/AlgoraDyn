#include <iostream>
#include <fstream>
#include <string>

#include "graph.dyn/dynamicdigraph.h"
#include "io/konectnetworkreader.h"

using namespace Algora;

bool readDynamicGraph(DynamicDiGraph &dyGraph, const std::string &filename);

int main(int argc, char *argv[]) {

    if (argc < 2) {
        std::cerr << "Please provide an input file, e.g.\n    "
            << argv[0]
            << " dynamic.graph\n";
        return 1;
    }

    DynamicDiGraph dyGraph;
    std::cout << "Reading dynamic graph " << argv[1] << "..." << std::endl;
    if (!readDynamicGraph(dyGraph, argv[1])) {
        return 1;
    }
    std::cout << "\n";

    // reset to empty graph
    dyGraph.resetToBigBang();
    // get pointer to static graph that is dynamically updated
    auto graph = dyGraph.getDiGraph();

    int me = -1; // dummy identifier for observer
    // set to true if you want to know what's going on in the graph
    constexpr bool logGraphUpdates = true;

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

    // Use dyGraph.applyNextDelta() to apply all updates with the next timestamp.
    // Use dyGraph.applyNextOperation() if you just want the next operation.
    while(dyGraph.applyNextOperation()) {
        if (dyGraph.lastOpWasVertexAddition()) {
            std::cout << "Last operation was a vertex insertion.\n";
        } else if (dyGraph.lastOpWasVertexRemoval()) {
            std::cout << "Last operation was a vertex deletion.\n";
        } else if (dyGraph.lastOpWasArcAddition()) {
            std::cout << "Last operation was an arc insertion.\n";
        } else if (dyGraph.lastOpWasArcRemoval()) {
            std::cout << "Last operation was an arc deletion.\n";
        } else {
            std::cout << "Last operation was something else.\n";
        }
        if (dyGraph.lastOpWasMultiple()) {
            std::cout << "Last operation was part of a group of operations.\n\n";
        } else {
            std::cout << "\n";
        }
    }


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
