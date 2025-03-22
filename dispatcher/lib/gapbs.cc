#include <dispatcher/api.hh>
#include <dispatcher/app.hh>
#include <dispatcher/common.hh>
#include <filesystem>
#include <iostream>
#include <vector>

#include "../deps/gapbs/src/benchmark.h"
#include "../deps/gapbs/src/bitmap.h"
#include "../deps/gapbs/src/builder.h"
#include "../deps/gapbs/src/command_line.h"
#include "../deps/gapbs/src/graph.h"
#include "../deps/gapbs/src/platform_atomics.h"
#include "../deps/gapbs/src/pvector.h"
#include "../deps/gapbs/src/sliding_queue.h"
#include "../deps/gapbs/src/timer.h"

extern "C" {

Graph graph;
WGraph wgraph;

using GReader = Reader<NodeID, NodeID, WeightT>;
using WGReader = Reader<NodeID, WNode, WeightT>;

int dp_addon_gapbs_init(const std::string path) {
    std::filesystem::path filePath = path;
    if (filePath.extension() == ".sg") {
        GReader r(path);
        graph = r.ReadSerializedGraph();
    } else if (filePath.extension() == ".wsg") {
        WGReader r(path);
        wgraph = r.ReadSerializedGraph();
    } else {
        return 1;
    }
    return 0;
}
}