#include <iostream>
#include <fstream>
#include <string>
#include <cstring>

//#include "graph.dyn/dynamicdigraph.h"
#include "graph.dyn/dynamicweighteddigraph.h"
#include "io/konectnetworkreader.h"

using namespace Algora;
using DynamicWDiGraph = DynamicWeightedDiGraph<unsigned long>;

bool readDynamicGraph(DynamicWDiGraph &dyGraph, const std::string &filename, bool weighted);

int main(int argc, char *argv[]) {

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input graph> [ weighted ]\n";
        return 1;
    }

    bool weighted = false;
    if (argc > 2) {
        if (strcmp(argv[2], "weighted") == 0) {
            weighted = true;
        } else {
            std::cerr << "Second argument can only be 'weighted'.\nUsage: " << argv[0] << " <input graph> [ weighted ]\n";
        }
    }

    DynamicWDiGraph dyGraph;
    std::cout << "Reading dynamic graph " << argv[1] << "..." << std::endl;
    if (!readDynamicGraph(dyGraph, argv[1], weighted)) {
        return 1;
    }
    std::cout << "\n";

    // reset to empty graph
    dyGraph.resetToBigBang();
    // get pointer to static graph that is dynamically updated
    auto graph = dyGraph.getDiGraph();

    int me = -1; // dummy identifier for observer

    graph->onVertexAdd(&me, [&](Vertex *v) {
            std::cout << "Added vertex" //<< v->getName()
                << " with id " << v->getId()
                << " at time " << dyGraph.getCurrentTime() << ".\n"; });
    graph->onVertexRemove(&me, [&](Vertex *v) {
            std::cout << "Removing vertex" //<< v->getName()
                << "with id " << v->getId()
                << " at time " << dyGraph.getCurrentTime() << ".\n"; });
    graph->onArcAdd(&me, [&](Arc *a) {
            std::cout << "Added " << (a->isDirected() ? "arc " : "edge ") << a
                << " with id " << a->getId()
                << " at time " << dyGraph.getCurrentTime() << ".\n"; });
    graph->onArcRemove(&me, [&](Arc *a) {
            std::cout << "Removing " << (a->isDirected() ? "arc " : "edge ") << a
                << " with id " << a->getId()
                << " at time " << dyGraph.getCurrentTime() << ".\n"; });

    if (weighted) {
        dyGraph.getArcWeights()->onPropertyChange(&me,
                [&](GraphArtifact *ga, auto oldValue, auto newValue) {
            std::cout << "Weight of " << ga->getId()
                << " changed from " << oldValue << " to " << newValue
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
        } else if (dyGraph.lastOpWasEdgeAddition()) {
            std::cout << "Last operation was an edge insertion.\n";
        } else if (dyGraph.lastOpWasEdgeRemoval()) {
            std::cout << "Last operation was an edge deletion.\n";
        } else if (dyGraph.lastOpWasArcWeightChange()) {
            std::cout << "Last operation was a weight change.\n";
        } else {
            std::cout << "Last operation was something else.\n";
        }
        if (dyGraph.lastOpWasMultiple()) {
            std::cout << "Last operation was part of a group of operations.\n\n";
        } else {
            std::cout << "\n";
        }
    }


     // remove observers
     graph->removeOnVertexAdd(&me);
     graph->removeOnVertexRemove(&me);
     graph->removeOnArcAdd(&me);
     graph->removeOnArcRemove(&me);
     if (weighted) {
        dyGraph.getArcWeights()->removeOnPropertyChange(&me);
     }

    return 0;
}

// use DynamicDiGraph instead of DynamicWeightedDigraph if you need an unweighted graph
bool readDynamicGraph(DynamicWDiGraph &dyGraph, const std::string &filename, bool weighted) {
    // create reader instance
    KonectNetworkReader reader;
    // use this version to insert all vertices at the very beginning
    // KonectNetworkReader reader(true);
    reader.setAllUndirected(true);
    std::ifstream input(filename, std::ifstream::in);
    if (!input) {
      std::cerr << "Could not open input file " << filename << std::endl;
      return false;
    }
    reader.setInputStream(&input);
    reader.setProgressStream(&std::cout);
    if (reader.isGraphAvailable()) {
      if ((!weighted && reader.provideDynamicDiGraph(&dyGraph))
              || (weighted && reader.provideDynamicWeightedDiGraph(&dyGraph))) {
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
