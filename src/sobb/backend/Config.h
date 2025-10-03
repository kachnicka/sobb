#pragma once

#include <berries/util/types.h>
#include <string>

namespace backend::config {

enum class BV {
    eNone,
    eAABB,
    eOBB,

    eDOP14,
    eDOP14split,

    eSOBB_d,
    eSOBB_d32,
    eSOBB_d48,
    eSOBB_d64,

    eSOBB_i32,
    eSOBB_i48,
    eSOBB_i64,
};

enum class SpaceFilling {
    eMorton32,
};

enum class NodeLayout {
    eDefault,
    eBVH2,
};

enum class InitialClusters {
    eTriangles,
};

struct PLOC {
    BV bv { BV::eNone };
    struct Shaders {
        std::string initialClusters;
        std::string copyClusters;
        std::string iterations;

        bool operator==(Shaders const& rhs) const = default;
    } shader;
    SpaceFilling sfc { SpaceFilling::eMorton32 };
    InitialClusters ic { InitialClusters::eTriangles };
    u32 radius { 16 };

    bool operator==(PLOC const& rhs) const = default;
};

struct Collapsing {
    BV bv { BV::eNone };
    struct Shaders {
        std::string collapse;

        bool operator==(Shaders const& rhs) const = default;
    } shader;
    u32 maxLeafSize { 15 };
    float c_t { 3.f };
    float c_i { 2.f };

    bool operator==(Collapsing const& rhs) const = default;
};

struct Transformation {
    BV bv { BV::eNone };
    struct Shaders {
        std::string transform;

        bool operator==(Shaders const& rhs) const = default;
    } shader;

    bool operator==(Transformation const& rhs) const = default;
};

struct Rearrangement {
    BV bv { BV::eNone };
    struct Shaders {
        std::string rearrange;

        bool operator==(Shaders const& rhs) const = default;
    } shader;
    NodeLayout layout { NodeLayout::eBVH2 };

    bool operator==(Rearrangement const& rhs) const = default;
};

struct Tracer {
    struct PersistentThreads {
        u32 workgroupCount { 512 };
        u32 warpsPerWorkgroup { 6 };

        bool operator==(PersistentThreads const& rhs) const = default;
    };

    BV bv { BV::eNone };
    struct Shaders {
        std::string genPrimary;
        std::string traceRays;
        std::string shadeAndCast;

        std::string traceRays_bv;
        std::string shadeAndCast_bv;
        std::string traceRays_int;
        std::string shadeAndCast_int;

        bool operator==(Shaders const& rhs) const = default;
    } shader;
    PersistentThreads rPrimary;
    PersistentThreads rSecondary;

    u32 bvDepth { 1 };
    bool bvRenderTriangles { false };

    bool operator==(Tracer const& rhs) const = default;
};

struct Stats {
    BV bv { BV::eNone };
    float c_t { 1.f };
    float c_i { 1.f };

    bool operator==(Stats const& rhs) const = default;
};

struct BVHPipeline {
    std::string name;

    PLOC plocpp;
    Collapsing collapsing;
    Transformation transformation;
    Rearrangement rearrangement;
    Tracer tracer;

    Stats stats;
};

}
