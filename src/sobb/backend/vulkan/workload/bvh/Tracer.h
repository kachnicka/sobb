#pragma once

#include "../../../Config.h"
#include "../../../Stats.h"
#include "../../VCtx.h"
#include "../../VulkanState.h"
#include "Types.h"
#include <berries/util/ConstexprEnumMap.h>
#include <vLime/Compute.h>
#include <vLime/Memory.h>
#include <vLime/Timestamp.h>
#include <vLime/vLime.h>

namespace backend::vulkan::bvh {

struct Tracer {
    explicit Tracer(VCtx ctx);

    [[nodiscard]] config::Tracer const& GetConfig() const
    {
        return config;
    }

    void Trace(vk::CommandBuffer commandBuffer, config::Tracer const& traceCfg, TraceRuntime const& trt, Bvh const& inputBvh);
    [[nodiscard]] stats::Trace GetStats() const;
    void SetVisualizationMode(State::VisMode mode);

public:
    std::array<f32, 4> dirLight;

private:
    VCtx ctx;
    config::Tracer config;
    lime::SingleTimer timestamp;

    enum class Pipeline {
        eTracePrimary,
        eTraceSecondary,
        eGenPrimary,
        eShadeAndCast,
        eDebug,
        eCount,
    };
    berry::ConstexprEnumMap<Pipeline, lime::PipelineCompute> pipelines;

    struct {
        struct {
            u32 x { 32 };
            u32 y { 32 };
        } primaryRaysTileSize;

        u32 workgroupSize { 0 };
        u32 rayCount { 0 };

        config::NodeLayout bvhMemoryLayout { config::NodeLayout::eBVH2 };
        State::VisMode visMode { State::VisMode::ePathTracing };

        bool reloadPipelines { true };
        bool reallocRayBuffers { false };
    } metadata;

    enum class Buffer {
        eScheduler,
    };

    lime::Buffer bStaging;
    lime::Buffer bTimes;
    lime::Buffer bStats;
    lime::Buffer bRayMetadata[2];

    lime::Buffer bRay[2];
    lime::Buffer bRayPayload[2];
    lime::Buffer bTraceResult;

    vk::UniqueDescriptorPool dPool;
    vk::DescriptorSet dSet;

    void checkForShaderHotReload();
    void reloadPipelines();
    void dSetUpdate(vk::ImageView targetImageView, lime::Buffer::Detail const& camera) const;
    void allocRayBuffers(uint32_t rayCount);
    void allocStatic();

    void trace_separate(vk::CommandBuffer commandBuffer, TraceRuntime const& trt, Bvh const& inputBvh);
    void trace_separate_bv(vk::CommandBuffer commandBuffer, TraceRuntime const& trt, Bvh const& inputBvh);
    void debugTrace(vk::CommandBuffer commandBuffer, TraceRuntime const& trt, Bvh const& inputBvh);

    void freeRayBuffers();
    void freeAll();
};

}
