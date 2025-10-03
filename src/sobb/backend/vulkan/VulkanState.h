#pragma once

#include "../Config.h"
#include <array>
#include <berries/util/types.h>
#include <berries/util/EnumValue.h>
#include <utility>

namespace backend::vulkan {

struct State {
    enum class Renderer {
        ePathTracingCompute,
    };

    enum class VisMode {
        ePathTracing,
        eBVVisualization,
        eBVIntersections,
    };

    enum class BvhBuilder {
        ePLOCpp,
    };

    enum class MortonCode {
        eMorton30,
        eMorton60,
    };

    enum class Centroid {
        eTriangle,
        eAABB,
    };

    double oneSecondAcc { 0. };
    bool oncePerSecond { false };
    double ms250Acc { 0. };
    bool oncePer250ms { false };

    bool vSync { true };

    berry::EnumValue<Renderer> selectedRenderer { Renderer::ePathTracingCompute };
    berry::EnumValue<VisMode> selectedVisMode { VisMode::ePathTracing };
    int selectedBvhBuilder_TMP { std::to_underlying(BvhBuilder::ePLOCpp) };

    u32 selectedNode { static_cast<u32>(-1) };
    f32 debugViewHeatMapScale { 256.f };
    bool debugViewOnlySelected { false };
    bool debugViewShowScene { true };
    bool debugViewShowHeatMap { false };
    bool debugViewShowWireframe { false };

    bool bvExplorerFitScene { false };
    std::array<f32, 12> bvExplorerData;

    bool resetAccumulation { true };
    u32 samplesToComputeThisFrame { 1 };
    u32 samplesPerPixel { 1 };
    u32 samplesComputed { 1 };
    u32 ptDepth { 7 };
    std::array<f32, 4> dirLight { .57735f, .57735f, .57735f, 1.f };

    config::BVHPipeline ptComputeConfig;

    bool readBackRayCount { false };

    u32 bvhCollapsing_c_t { 3 };
    u32 bvhCollapsing_c_i { 2 };
};

}
