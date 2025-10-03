#include "Transformation.h"

#include <final/shared/data_bvh.h>
#include <final/shared/data_plocpp.h>
#include <vLime/ComputeHelpers.h>

namespace backend::vulkan::bvh {

static u32 CreateSpecializationConstants(vk::PhysicalDevice pd)
{
    auto const prop2 { pd.getProperties2() };
    return std::min(512u, prop2.properties.limits.maxComputeWorkGroupSize[0]);
}

Transformation::Transformation(VCtx ctx)
    : ctx(ctx)
    , timestamps(ctx.d, ctx.pd)
{
}

bool Transformation::CheckForShaderHotReload()
{
    if (config.bv == config::BV::eNone)
        return false;
    return pTransform.Update(ctx.d, ctx.sCache);
}

Bvh Transformation::GetBVH() const
{
    return {
        .bvh = buffersOut.at(Buffer::eBVH).getDeviceAddress(ctx.d),
        .triangles = metadata.bvhTriangles,
        .triangleIDs = metadata.bvhTriangleIDs,
        .nodeCountLeaf = metadata.nodeCountLeaf,
        .nodeCountTotal = metadata.nodeCountTotal,
        .bv = config.bv,
    };
}

void Transformation::Compute(vk::CommandBuffer commandBuffer, Bvh const& inputBvh, vk::DeviceAddress geometryDescriptor)
{
    metadata.nodeCountLeaf = inputBvh.nodeCountLeaf;
    metadata.nodeCountTotal = inputBvh.nodeCountTotal;
    metadata.bvhTriangles = inputBvh.triangles;
    metadata.bvhTriangleIDs = inputBvh.triangleIDs;

    reloadPipelines();
    alloc();
    lime::compute::fillZeros(commandBuffer, buffersIntermediate[Buffer::eTraversalCounters]);
    if (config.bv == config::BV::eOBB)
        lime::compute::fillZeros(commandBuffer, buffersIntermediate[Buffer::eScheduler]);

    if (buffersOut.count(Buffer::eStats_TMP) > 0)
        lime::compute::fillZeros(commandBuffer, buffersOut[Buffer::eStats_TMP]);

    timestamps.Reset(commandBuffer);
    vk::MemoryBarrier bufferWriteBarrier { .srcAccessMask = vk::AccessFlagBits::eTransferWrite, .dstAccessMask = vk::AccessFlagBits::eShaderRead };
    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags(), bufferWriteBarrier, nullptr, nullptr);
    timestamps.Begin(commandBuffer);
    switch (config.bv) {
    case config::BV::eDOP14:
        transform_dop14(commandBuffer, inputBvh, geometryDescriptor);
        break;
    case config::BV::eOBB:
        transform_obb(commandBuffer, inputBvh, geometryDescriptor);
        break;
    case config::BV::eSOBB_d32:
    case config::BV::eSOBB_d48:
    case config::BV::eSOBB_d64:
    case config::BV::eSOBB_i32:
    case config::BV::eSOBB_i48:
    case config::BV::eSOBB_i64:
        transform_sobb(commandBuffer, inputBvh, geometryDescriptor);
        break;
    default:
        break;
    }
    timestamps.End(commandBuffer);
}

void Transformation::ReadRuntimeData()
{
}

stats::Transformation Transformation::GatherStats(BvhStats const& bvhStats)
{
    stats::Transformation stats;
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

void Transformation::reloadPipelines()
{
    metadata.workgroupSize = CreateSpecializationConstants(ctx.pd);

    static std::array<vk::SpecializationMapEntry, 1> constexpr entries {
        vk::SpecializationMapEntry { 0, 0, sizeof(u32) },
    };
    vk::SpecializationInfo sInfo { 1, entries.data(), 4, &metadata.workgroupSize };

    pTransform = { ctx.d, ctx.sCache, config.shader.transform, sInfo };
}

void Transformation::transform_dop14(vk::CommandBuffer commandBuffer, Bvh const& inputBvh, vk::DeviceAddress geometryDescriptor)
{
    vk::MemoryBarrier memoryBarrierCompute { .srcAccessMask = vk::AccessFlagBits::eShaderWrite, .dstAccessMask = vk::AccessFlagBits::eShaderRead };
    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags(), memoryBarrierCompute, nullptr, nullptr);

    data_plocpp::PC_TransformToDOP pc {
        .bvhAddress = inputBvh.bvh,
        .bvhTriangleIndicesAddress = inputBvh.triangleIDs,
        .bvhDOPAddress = buffersOut[Buffer::eBVH].getDeviceAddress(ctx.d),

        .bvhNodeCountsAddress = 0,

        .geometryDescriptorAddress = geometryDescriptor,
        .countersAddress = buffersIntermediate[Buffer::eTraversalCounters].getDeviceAddress(ctx.d),

        .leafNodeCount = inputBvh.nodeCountLeaf,
    };

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pTransform.get());
    commandBuffer.pushConstants(pTransform.layout.pipeline.get(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(pc), &pc);
    commandBuffer.dispatch(lime::divCeil(inputBvh.nodeCountLeaf, metadata.workgroupSize), 1, 1);
}

void Transformation::transform_obb(vk::CommandBuffer commandBuffer, Bvh const& inputBvh, vk::DeviceAddress geometryDescriptor)
{
    vk::MemoryBarrier memoryBarrierCompute { .srcAccessMask = vk::AccessFlagBits::eShaderWrite, .dstAccessMask = vk::AccessFlagBits::eShaderRead };
    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags(), memoryBarrierCompute, nullptr, nullptr);

    data_plocpp::PC_TransformToOBB pc {
        .bvhInAddress = inputBvh.bvh,
        .bvhInTriangleIndicesAddress = inputBvh.triangleIDs,
        .bvhOutAddress = buffersOut[Buffer::eBVH].getDeviceAddress(ctx.d),

        .ditoPointsAddress = buffersIntermediate[Buffer::eDitoPoints].getDeviceAddress(ctx.d),
        .obbAddress = buffersIntermediate[Buffer::eOBB].getDeviceAddress(ctx.d),

        .geometryDescriptorAddress = geometryDescriptor,
        .traversalCounterAddress = buffersIntermediate[Buffer::eTraversalCounters].getDeviceAddress(ctx.d),
        .schedulerDataAddress = buffersIntermediate[Buffer::eScheduler].getDeviceAddress(ctx.d),
        .timesAddress_TMP = buffersIntermediate[Buffer::eTimes_TMP].getDeviceAddress(ctx.d),

        .leafNodeCount = inputBvh.nodeCountLeaf,
    };

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pTransform.get());
    commandBuffer.pushConstants(pTransform.layout.pipeline.get(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(pc), &pc);
    commandBuffer.dispatch(std::max(2048u, lime::divCeil(inputBvh.nodeCountLeaf, metadata.workgroupSize)), 1, 1);
}

void Transformation::transform_sobb(vk::CommandBuffer commandBuffer, Bvh const& inputBvh, vk::DeviceAddress geometryDescriptor)
{
    vk::MemoryBarrier memoryBarrierCompute { .srcAccessMask = vk::AccessFlagBits::eShaderWrite, .dstAccessMask = vk::AccessFlagBits::eShaderRead };
    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags(), memoryBarrierCompute, nullptr, nullptr);

    data_plocpp::PC_TransformToSOBB pc {
        .bvhAddress = inputBvh.bvh,
        .bvhTriangleIndicesAddress = inputBvh.triangleIDs,
        .bvhSOBBAddress = buffersOut[Buffer::eBVH].getDeviceAddress(ctx.d),

        .geometryDescriptorAddress = geometryDescriptor,
        .countersAddress = buffersIntermediate[Buffer::eTraversalCounters].getDeviceAddress(ctx.d),

        .dopBaseAddress = buffersIntermediate[Buffer::eBaseDOP].getDeviceAddress(ctx.d),
        .dopRefAddress = buffersIntermediate[Buffer::eDOPRef].getDeviceAddress(ctx.d),
        .statsAddress = buffersOut[Buffer::eStats_TMP].getDeviceAddress(ctx.d),

        .leafNodeCount = inputBvh.nodeCountLeaf,
    };

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pTransform.get());
    commandBuffer.pushConstants(pTransform.layout.pipeline.get(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(pc), &pc);
    commandBuffer.dispatch(lime::divCeil(inputBvh.nodeCountLeaf, metadata.workgroupSize), 1, 1);
}

void Transformation::freeIntermediate()
{
    buffersIntermediate.clear();
}

void Transformation::freeAll()
{
    freeIntermediate();
    buffersOut.clear();
}

void Transformation::alloc()
{
    freeAll();
    ctx.memory.cleanUp();

    auto const getNodeSize { [](config::BV bv) -> vk::DeviceSize {
        switch (bv) {
        case config::BV::eDOP14:
            return sizeof(data_bvh::NodeBVH2_DOP14);
        case config::BV::eOBB:
            return sizeof(data_bvh::NodeBVH2_OBB);
        case config::BV::eSOBB_d32:
        case config::BV::eSOBB_d48:
        case config::BV::eSOBB_d64:
            return sizeof(data_bvh::NodeBVH2_SOBB);
        case config::BV::eSOBB_i32:
        case config::BV::eSOBB_i48:
        case config::BV::eSOBB_i64:
            return sizeof(data_bvh::NodeBVH2_SOBBi);
        default:
            break;
        }
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

    cInfo.size = getNodeSize(config.bv) * metadata.nodeCountTotal;
    buffersOut[Buffer::eBVH] = ctx.memory.alloc(aReq, cInfo, "bvh_transformed");

    cInfo.size = sizeof(u32) * metadata.nodeCountTotal;
    buffersIntermediate[Buffer::eTraversalCounters] = ctx.memory.alloc(aReq, cInfo, "transformation_traversal_counters");

    if (config.bv == config::BV::eOBB) {
        cInfo.size = sizeof(f32) * 3 * 14 * metadata.nodeCountTotal;
        buffersIntermediate[Buffer::eDitoPoints] = ctx.memory.alloc(aReq, cInfo, "transformation_obb_dito_points");
        cInfo.size = sizeof(f32) * 3 * 5 * metadata.nodeCountTotal;
        buffersIntermediate[Buffer::eOBB] = ctx.memory.alloc(aReq, cInfo, "transformation_obb");
        cInfo.size = sizeof(u32) * 5;
        buffersIntermediate[Buffer::eScheduler] = ctx.memory.alloc(aReq, cInfo, "transformation_obb_scheduler");

        buffersIntermediate[Buffer::eTimes_TMP] = ctx.memory.alloc(
            { .memoryUsage = lime::DeviceMemoryUsage::eDeviceToHost },
            { .size = sizeof(u64) * 5, .usage = bfub::eStorageBuffer | bfub::eShaderDeviceAddress | bfub::eTransferSrc | bfub::eTransferDst },
            "transformation_times");
    }

    auto const getDopSize { [](config::BV bv) -> vk::DeviceSize {
        switch (bv) {
        case config::BV::eSOBB_i32:
        case config::BV::eSOBB_d32:
            return 32;
        case config::BV::eSOBB_i48:
        case config::BV::eSOBB_d48:
            return 48;
        case config::BV::eSOBB_i64:
        case config::BV::eSOBB_d64:
            return 64;
        default:
            break;
        }
        return 0;
    } };

    if (auto const dopSize { getDopSize(config.bv) }) {
        cInfo.size = sizeof(f32) * dopSize * metadata.nodeCountLeaf;
        buffersIntermediate[Buffer::eBaseDOP] = ctx.memory.alloc(aReq, cInfo, "transformation_sobb_dop");
        cInfo.size = sizeof(u32) * metadata.nodeCountTotal;
        buffersIntermediate[Buffer::eDOPRef] = ctx.memory.alloc(aReq, cInfo, "transformation_sobb_dop_ref");
    }

    cInfo.size = sizeof(f32) * 20;
    buffersOut[Buffer::eStats_TMP] = ctx.memory.alloc(aReq, cInfo, "transformation_sobb_dop_stats");
}

}
