#include "Stats.h"

#include <final/shared/data_bvh.h>

namespace backend::vulkan::bvh {

static u32 CreateSpecializationConstants(vk::PhysicalDevice pd)
{
    auto const prop2 { pd.getProperties2() };
    return prop2.properties.limits.maxComputeWorkGroupSize[0];
}

Stats::Stats(VCtx ctx)
    : ctx(ctx)
{
    alloc();
}

void Stats::Compute(vk::CommandBuffer commandBuffer, config::Stats const& buildCfg, Bvh const& bvh)
{
    config = buildCfg;
    reloadPipelines(bvh.layout);
    stats(commandBuffer, bvh);
}

void Stats::reloadPipelines(config::NodeLayout layout)
{
    workgroupSize = CreateSpecializationConstants(ctx.pd);

    static std::array<vk::SpecializationMapEntry, 1> constexpr entries {
        vk::SpecializationMapEntry { 0, 0, sizeof(u32) },
    };
    vk::SpecializationInfo sInfo { 1, entries.data(), 4, &workgroupSize };

    switch (config.bv) {
    case config::BV::eAABB:
        switch (layout) {
        case config::NodeLayout::eDefault:
            pStats = { ctx.d, ctx.sCache, "final/stats_bvh2_aabb.comp.spv", sInfo };
            break;
        case config::NodeLayout::eBVH2:
            pStats = { ctx.d, ctx.sCache, "final/stats_bvh2_aabb_c.comp.spv", sInfo };
            break;
        }
        break;
    case config::BV::eDOP14:
        switch (layout) {
        case config::NodeLayout::eDefault:
            pStats = { ctx.d, ctx.sCache, "final/stats_bvh2_dop14.comp.spv", sInfo };
            break;
        case config::NodeLayout::eBVH2:
            pStats = { ctx.d, ctx.sCache, "final/stats_bvh2_dop14_c.comp.spv", sInfo };
            break;
        default:;
        }
        break;
    case config::BV::eOBB:
        switch (layout) {
        case config::NodeLayout::eDefault:
            pStats = { ctx.d, ctx.sCache, "final/stats_bvh2_obb.comp.spv", sInfo };
            break;
        case config::NodeLayout::eBVH2:
            pStats = { ctx.d, ctx.sCache, "final/stats_bvh2_obb_c.comp.spv", sInfo };
            break;
        default:;
        }
        break;
    case config::BV::eSOBB_d:
    case config::BV::eSOBB_d32:
    case config::BV::eSOBB_d48:
    case config::BV::eSOBB_d64:
        switch (layout) {
        case config::NodeLayout::eDefault:
            pStats = { ctx.d, ctx.sCache, "final/stats_bvh2_sobb_d.comp.spv", sInfo };
            break;
        case config::NodeLayout::eBVH2:
            pStats = { ctx.d, ctx.sCache, "final/stats_bvh2_sobb_d_c.comp.spv", sInfo };
            break;
        default:;
        }
        break;
    case config::BV::eSOBB_i32:
        switch (layout) {
        case config::NodeLayout::eDefault:
            pStats = { ctx.d, ctx.sCache, "final/stats_bvh2_sobb_i32.comp.spv", sInfo };
            break;
        case config::NodeLayout::eBVH2:
            pStats = { ctx.d, ctx.sCache, "final/stats_bvh2_sobb_i32_c.comp.spv", sInfo };
            break;
        default:;
        }
        break;
    case config::BV::eSOBB_i48:
        switch (layout) {
        case config::NodeLayout::eDefault:
            pStats = { ctx.d, ctx.sCache, "final/stats_bvh2_sobb_i48.comp.spv", sInfo };
            break;
        case config::NodeLayout::eBVH2:
            pStats = { ctx.d, ctx.sCache, "final/stats_bvh2_sobb_i48_c.comp.spv", sInfo };
            break;
        default:;
        }
        break;
    case config::BV::eSOBB_i64:
        switch (layout) {
        case config::NodeLayout::eDefault:
            pStats = { ctx.d, ctx.sCache, "final/stats_bvh2_sobb_i64.comp.spv", sInfo };
            break;
        case config::NodeLayout::eBVH2:
            pStats = { ctx.d, ctx.sCache, "final/stats_bvh2_sobb_i64_c.comp.spv", sInfo };
            break;
        default:;
        }
        break;
    default:
        pStats = {};
    }
}

void Stats::alloc()
{
    freeAll();
    ctx.memory.cleanUp();

    using bfub = vk::BufferUsageFlagBits;
    lime::AllocRequirements aReq {
        .memoryUsage = lime::DeviceMemoryUsage::eDeviceToHost,
    };
    vk::BufferCreateInfo cInfo {
        .size = sizeof(BvhStats),
        .usage = bfub::eStorageBuffer | bfub::eShaderDeviceAddress | bfub::eTransferSrc | bfub::eTransferDst,
    };

    bStats = ctx.memory.alloc(aReq, cInfo, "bvh_stats");
    data = static_cast<BvhStats*>(bStats.getMapping());
}

void Stats::freeAll()
{
    bStats.reset();
}

void Stats::stats(vk::CommandBuffer commandBuffer, Bvh const& bvh)
{
    *data = BvhStats {};

    if (!pStats.get())
        return;

    data_bvh::PC_BvhStats pc {
        .bvhAddress = bvh.bvh,
        .bvhAuxAddress = bvh.bvhAux,
        .resultBufferAddress = bStats.getDeviceAddress(ctx.d),
        .nodeCount = bvh.nodeCountTotal,
        .c_t = config.c_t,
        .c_i = config.c_i,
        .sceneAABBSurfaceArea = metadata.sceneAabbSurfaceArea,
    };
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pStats.get());
    commandBuffer.pushConstants(pStats.layout.pipeline.get(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(pc), &pc);
    commandBuffer.dispatch(lime::divCeil(pc.nodeCount, workgroupSize), 1, 1);
}

}
