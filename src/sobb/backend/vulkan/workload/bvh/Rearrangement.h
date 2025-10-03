#pragma once

#include "../../../Config.h"
#include "../../../Stats.h"
#include "../../VCtx.h"
#include "Types.h"
#include <vLime/Compute.h>
#include <vLime/Memory.h>
#include <vLime/Timestamp.h>
#include <vLime/vLime.h>

namespace backend::vulkan::bvh {

struct Rearrangement {
    explicit Rearrangement(VCtx ctx);

    [[nodiscard]] Bvh GetBVH() const;
    [[nodiscard]] bool NeedsRecompute(config::Rearrangement const& buildConfig)
    {
        auto const cfgChanged { config != buildConfig };
        config = buildConfig;
        return cfgChanged && config.bv != config::BV::eNone;
    }
    [[nodiscard]] bool CheckForShaderHotReload();

    void Compute(vk::CommandBuffer commandBuffer, Bvh const& inputBvh);
    void ReadRuntimeData();
    [[nodiscard]] stats::Rearrangement GatherStats(BvhStats const& bvhStats);

private:
    VCtx ctx;
    config::Rearrangement config;

    lime::PipelineCompute pRearrange;

    struct Metadata {
        u32 nodeCountLeaf { 0 };
        u32 nodeCountTotal { 0 };
        vk::DeviceAddress bvhTriangles { 0 };
        vk::DeviceAddress bvhTriangleIDs { 0 };

        u32 workgroupSize { 0 };
    } metadata;

    enum class Buffer {
        eBVH,
        eSplit,
        eWorkBuffer,
        eRuntimeData,
    };

    // lime::Buffer stagingBuffer;
    std::unordered_map<Buffer, lime::Buffer> buffersOut;
    std::unordered_map<Buffer, lime::Buffer> buffersIntermediate;

    void reloadPipelines();
    void alloc();
public:
    void freeIntermediate();
    void freeAll();
private:

    void rearrange(vk::CommandBuffer commandBuffer, Bvh const& inputBvh);

    lime::SingleTimer timestamps;
};

}
