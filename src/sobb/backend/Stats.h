#pragma once

#include <berries/lib_helper/spdlog.h>
#include <vLime/types.h>

#include <vector>

namespace backend::stats {

struct PLOC {
    std::vector<f32> times;
    f32 timeTotal { 0.f };

    u32 iterationCount { 0 };
    f32 saIntersect { 0.f };
    f32 saTraverse { 0.f };
    f32 costTotal { 0.f };

    u32 nodeCountTotal { 0 };
    u32 leafSizeMin { 0 };
    u32 leafSizeMax { 0 };
    f32 leafSizeAvg { 0.f };

    void print() const
    {
        berry::log::info("  PLOC:");
        berry::log::info("    Time total: {:.2f} ms", timeTotal);
        berry::log::info("{:>14.2f} ms  - init. clusters, woopify", times[0]);
        berry::log::info("{:>14.2f} ms  - radix sort", times[1]);
        berry::log::info("{:>14.2f} ms  - copy clusters", times[2]);
        berry::log::info("{:>14.2f} ms  - PLOC iterations", times[3]);
        berry::log::info("    Iteration count: {}", iterationCount);
        berry::log::info("    Cost total: {:.2f}", costTotal);
        berry::log::info("{:>17.2f}  - area intersect", saIntersect);
        berry::log::info("{:>17.2f}  - area traverse", saTraverse);
        berry::log::info("    #Nodes total: {}", nodeCountTotal);
        berry::log::info("    Leaf size min: {}", leafSizeMin);
        berry::log::info("    Leaf size max: {}", leafSizeMax);
        berry::log::info("    Leaf size avg: {:.2f}", leafSizeAvg);
    }
};

struct Transformation {
    f32 timeTotal { 0.f };

    f32 saIntersect { 0.f };
    f32 saTraverse { 0.f };
    f32 costTotal { 0.f };

    u32 nodeCountTotal { 0 };
    u32 leafSizeMin { 0 };
    u32 leafSizeMax { 0 };
    f32 leafSizeAvg { 0.f };

    void print() const
    {
        berry::log::info("  Transformation:");
        berry::log::info("    Time total: {:.2f} ms", timeTotal);
        berry::log::info("    Cost total: {:.2f}", costTotal);
        berry::log::info("{:>17.2f}  - area intersect", saIntersect);
        berry::log::info("{:>17.2f}  - area traverse", saTraverse);
        berry::log::info("    #Nodes total: {}", nodeCountTotal);
        berry::log::info("    Leaf size min: {}", leafSizeMin);
        berry::log::info("    Leaf size max: {}", leafSizeMax);
        berry::log::info("    Leaf size avg: {:.2f}", leafSizeAvg);
    }
};

struct Collapsing {
    f32 timeTotal { 0.f };

    f32 saIntersect { 0.f };
    f32 saTraverse { 0.f };
    f32 costTotal { 0.f };

    u32 nodeCountTotal { 0 };
    u32 leafSizeMin { 0 };
    u32 leafSizeMax { 0 };
    f32 leafSizeAvg { 0.f };

    void print() const
    {
        berry::log::info("  Collapsing:");
        berry::log::info("    Time total: {:.2f} ms", timeTotal);
        berry::log::info("    Cost total: {:.2f}", costTotal);
        berry::log::info("{:>17.2f}  - area intersect", saIntersect);
        berry::log::info("{:>17.2f}  - area traverse", saTraverse);
        berry::log::info("    #Nodes total: {}", nodeCountTotal);
        berry::log::info("    Leaf size min: {}", leafSizeMin);
        berry::log::info("    Leaf size max: {}", leafSizeMax);
        berry::log::info("    Leaf size avg: {:.2f}", leafSizeAvg);
    }
};

struct Rearrangement {
    f32 timeTotal { 0.f };

    f32 saIntersect { 0.f };
    f32 saTraverse { 0.f };
    f32 costTotal { 0.f };

    u32 nodeCountTotal { 0 };
    u32 leafSizeMin { 0 };
    u32 leafSizeMax { 0 };
    f32 leafSizeAvg { 0.f };

    void print() const
    {
        berry::log::info("  Rearrangement:");
        berry::log::info("    Time total: {:.2f} ms", timeTotal);
        berry::log::info("    Cost total: {:.2f}", costTotal);
        berry::log::info("{:>17.2f}  - area intersect", saIntersect);
        berry::log::info("{:>17.2f}  - area traverse", saTraverse);
        berry::log::info("    #Nodes total: {}", nodeCountTotal);
        berry::log::info("    Leaf size min: {}", leafSizeMin);
        berry::log::info("    Leaf size max: {}", leafSizeMax);
        berry::log::info("    Leaf size avg: {:.2f}", leafSizeAvg);
    }
};

struct BVHPipeline {
    PLOC plocpp;
    Collapsing collapsing;
    Transformation transformation;
    Rearrangement rearrangement;

    void print() const
    {
        plocpp.print();
        collapsing.print();
        transformation.print();
        rearrangement.print();
    }

    void clear()
    {
        plocpp = {};
        collapsing = {};
        transformation = {};
        rearrangement = {};
    }
};

struct Trace {
    struct PerDepth {
        u32 rayCount { 0 };
        f32 traceTimeMs { 0.f };
    };
    std::array<PerDepth, 8> data;
    u32 testedNodes { 0 };
    u32 testedTriangles { 0 };
    u32 testedBVolumes { 0 };
};

}
