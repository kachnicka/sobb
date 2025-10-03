#include "Rearrangement.h"

#include <final/shared/data_bvh.h>
#include <final/shared/data_plocpp.h>
#include <vLime/ComputeHelpers.h>

namespace backend::vulkan::bvh {

static u32 CreateSpecializationConstants(vk::PhysicalDevice pd)
{
    auto const prop2 { pd.getProperties2() };
    return prop2.properties.limits.maxComputeWorkGroupSize[0];
}

Rearrangement::Rearrangement(VCtx ctx)
    : ctx(ctx)
    , timestamps(ctx.d, ctx.pd)
{
}

bool Rearrangement::CheckForShaderHotReload()
{
    if (config.bv == config::BV::eNone)
        return false;
    return pRearrange.Update(ctx.d, ctx.sCache);
}

Bvh Rearrangement::GetBVH() const
{
    return {
        .bvh = buffersOut.at(Buffer::eBVH).getDeviceAddress(ctx.d),
        .triangles = metadata.bvhTriangles,
        .triangleIDs = metadata.bvhTriangleIDs,
        .bvhAux = buffersOut.contains(Buffer::eSplit) ? buffersOut.at(Buffer::eSplit).getDeviceAddress(ctx.d) : 0,
        .nodeCountLeaf = metadata.nodeCountLeaf,
        .nodeCountTotal = metadata.nodeCountTotal,
        .bv = config.bv,
        .layout = config.layout,
    };
}

#define INVALID_VALUE 0x7FFFFFFF
void Rearrangement::Compute(vk::CommandBuffer commandBuffer, Bvh const& inputBvh)
{
    auto const estimateRearrangedNodeCount { [](u32 inTotal, config::NodeLayout layout) -> u32 {
        switch (layout) {
        case config::NodeLayout::eBVH2:
            return inTotal >> 1;
        default:;
        }
        return 0;
    } };

    metadata.nodeCountLeaf = inputBvh.nodeCountLeaf;
    metadata.nodeCountTotal = estimateRearrangedNodeCount(inputBvh.nodeCountTotal, config.layout);
    metadata.bvhTriangles = inputBvh.triangles;
    metadata.bvhTriangleIDs = inputBvh.triangleIDs;

    reloadPipelines();
    alloc();

    commandBuffer.fillBuffer(buffersIntermediate[Buffer::eRuntimeData].get(), 0, 4, 0);
    commandBuffer.fillBuffer(buffersIntermediate[Buffer::eRuntimeData].get(), 4, 8, 1);
    commandBuffer.fillBuffer(buffersIntermediate[Buffer::eWorkBuffer].get(), 0, 4, inputBvh.nodeCountTotal - 1);
    commandBuffer.fillBuffer(buffersIntermediate[Buffer::eWorkBuffer].get(), 4, 4, 0);
    commandBuffer.fillBuffer(buffersIntermediate[Buffer::eWorkBuffer].get(), 8, buffersIntermediate[Buffer::eWorkBuffer].getSizeInBytes() - 8, INVALID_VALUE);
    lime::compute::pBarrierTransferWrite(commandBuffer);

    timestamps.Reset(commandBuffer);
    timestamps.Begin(commandBuffer);
    rearrange(commandBuffer, inputBvh);
    timestamps.End(commandBuffer);
}

void Rearrangement::ReadRuntimeData()
{
    metadata.nodeCountTotal = static_cast<i32*>(buffersIntermediate[Buffer::eRuntimeData].getMapping())[2];
}

stats::Rearrangement Rearrangement::GatherStats(BvhStats const& bvhStats)
{
    static_cast<void>(bvhStats);
    stats::Rearrangement stats;
    stats.timeTotal = timestamps.ReadTimeNs() * 1e-6f;

    stats.saIntersect = bvhStats.saIntersect;
    stats.saTraverse = bvhStats.saTraverse;
    stats.costTotal = bvhStats.costIntersect + bvhStats.costTraverse;

    stats.nodeCountTotal = metadata.nodeCountTotal;
    stats.leafSizeMin = bvhStats.leafSizeMin;
    stats.leafSizeMax = bvhStats.leafSizeMax;
    stats.leafSizeAvg = static_cast<f32>(bvhStats.leafSizeSum) / static_cast<f32>(metadata.nodeCountLeaf);

    return stats;
}

void Rearrangement::reloadPipelines()
{
    metadata.workgroupSize = CreateSpecializationConstants(ctx.memory.pd);

    static std::array<vk::SpecializationMapEntry, 1> constexpr entries {
        vk::SpecializationMapEntry { 0, 0, sizeof(u32) },
    };
    vk::SpecializationInfo sInfo { 1, entries.data(), 4, &metadata.workgroupSize };

    pRearrange = { ctx.d, ctx.sCache, config.shader.rearrange, sInfo };
}

void Rearrangement::rearrange(vk::CommandBuffer commandBuffer, Bvh const& inputBvh)
{
    vk::MemoryBarrier memoryBarrierCompute { .srcAccessMask = vk::AccessFlagBits::eShaderWrite, .dstAccessMask = vk::AccessFlagBits::eShaderRead };
    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags(), memoryBarrierCompute, nullptr, nullptr);

    data_plocpp::PC_Rearrange pc {
        .bvhAddress = inputBvh.bvh,
        .bvhWideAddress = buffersOut[Buffer::eBVH].getDeviceAddress(ctx.d),
        .workBufferAddress = buffersIntermediate[Buffer::eWorkBuffer].getDeviceAddress(ctx.d),
        .runtimeDataAddress = buffersIntermediate[Buffer::eRuntimeData].getDeviceAddress(ctx.d),
        .auxBufferAddress = buffersOut.contains(Buffer::eSplit) ? buffersOut.at(Buffer::eSplit).getDeviceAddress(ctx.d) : 0,
        .leafNodeCount = inputBvh.nodeCountLeaf,
    };

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pRearrange.get());
    commandBuffer.pushConstants(pRearrange.layout.pipeline.get(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(pc), &pc);
    commandBuffer.dispatch(lime::divCeil(inputBvh.nodeCountLeaf, metadata.workgroupSize), 1, 1);
}

void Rearrangement::freeIntermediate()
{
    buffersIntermediate.clear();
}

void Rearrangement::freeAll()
{
    freeIntermediate();
    buffersOut.clear();
}

void Rearrangement::alloc()
{
    freeAll();
    ctx.memory.cleanUp();

    auto const getNodeSize { [](config::NodeLayout layout, config::BV bv) -> vk::DeviceSize {
        switch (layout) {
        case config::NodeLayout::eBVH2:
            switch (bv) {
            case config::BV::eAABB:
                return sizeof(data_bvh::NodeBVH2_AABB_c);
            case config::BV::eDOP14split:
                return sizeof(data_bvh::NodeBVH2_DOP3_c);
            case config::BV::eOBB:
                return sizeof(data_bvh::NodeBVH2_OBB_c);
            case config::BV::eDOP14:
                return sizeof(data_bvh::NodeBVH2_DOP14_c);
            case config::BV::eSOBB_d:
                return sizeof(data_bvh::NodeBVH2_SOBB_c);
            case config::BV::eSOBB_i32:
            case config::BV::eSOBB_i48:
            case config::BV::eSOBB_i64:
                return sizeof(data_bvh::NodeBVH2_SOBBi_c);
            default:;
            }
        default:;
        }
        berry::log::error("Rearrangement: unknown bvh node size.");
        return 0;
    } };

    using bfub = vk::BufferUsageFlagBits;
    lime::AllocRequirements aReq {
        .memoryUsage = lime::DeviceMemoryUsage::eDeviceOptimal,
        .additionalAlignment = 256,
    };
    vk::BufferCreateInfo cInfo {
        .size = 0,
        .usage = bfub::eStorageBuffer | bfub::eShaderDeviceAddress | bfub::eTransferSrc | bfub::eTransferDst,
    };

    cInfo.size = getNodeSize(config.layout, config.bv) * metadata.nodeCountTotal;
    buffersOut[Buffer::eBVH] = ctx.memory.alloc(aReq, cInfo, "bvh_rearranged");

    if (config.bv == config::BV::eDOP14split) {
        cInfo.size = sizeof(data_bvh::NodeBVH2_DOP14_SPLIT_c) * metadata.nodeCountTotal;
        buffersOut[Buffer::eSplit] = ctx.memory.alloc(aReq, cInfo, "bvh_rearranged_split");
    }

    cInfo.size = sizeof(u32) * 3;
    buffersIntermediate[Buffer::eRuntimeData] = ctx.memory.alloc(aReq, cInfo, "rearrangement_runtime");

    cInfo.size = sizeof(u32) * metadata.nodeCountLeaf * 2;
    buffersIntermediate[Buffer::eWorkBuffer] = ctx.memory.alloc(aReq, cInfo, "rearrangement_work_buffer");
}
}
