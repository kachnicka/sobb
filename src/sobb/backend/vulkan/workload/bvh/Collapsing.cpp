#include "Collapsing.h"

#include <final/shared/data_bvh.h>
#include <final/shared/data_plocpp.h>
#include <vLime/ComputeHelpers.h>

namespace backend::vulkan::bvh {

static u32 CreateSpecializationConstants(vk::PhysicalDevice pd)
{
    auto const prop2 { pd.getProperties2() };
    return prop2.properties.limits.maxComputeWorkGroupSize[0];
}

Collapsing::Collapsing(VCtx ctx)
    : ctx(ctx)
    , timestamps(ctx.d, ctx.pd)
{
}

bool Collapsing::CheckForShaderHotReload()
{
    return pCollapse.Update(ctx.d, ctx.sCache);
}

Bvh Collapsing::GetBVH() const
{
    return {
        .bvh = buffersOut.at(Buffer::eBVH).getDeviceAddress(ctx.d),
        .triangles = metadata.bvhTriangles != 0 ? metadata.bvhTriangles : buffersOut.at(Buffer::eBVHTriangles).getDeviceAddress(ctx.d),
        .triangleIDs = metadata.bvhTriangleIDs != 0 ? metadata.bvhTriangleIDs : buffersOut.at(Buffer::eBVHTriangleIDs).getDeviceAddress(ctx.d),
        .nodeCountLeaf = metadata.nodeCountLeaf,
        .nodeCountTotal = metadata.nodeCountTotal,
        .bv = config.bv,
        .layout = config::NodeLayout::eDefault,
    };
}

void Collapsing::Compute(vk::CommandBuffer commandBuffer, Bvh const& inputBvh, vk::DeviceAddress geometryDescriptor)
{
    // allocate full size based on input bvh
    metadata.nodeCountLeaf = inputBvh.nodeCountLeaf;
    metadata.nodeCountTotal = inputBvh.nodeCountTotal;

    metadata.bvhTriangleIDs = 0;

    if (config.maxLeafSize == 1) {
        metadata.bvhTriangles = inputBvh.triangles;
        metadata.bvhTriangleIDs = inputBvh.triangleIDs;
    } else {
        metadata.bvhTriangles = 0;
        metadata.bvhTriangleIDs = 0;
    }

    reloadPipelines();
    alloc();

    lime::compute::fillZeros(commandBuffer, buffersIntermediate[Buffer::eScheduler]);
    lime::compute::fillZeros(commandBuffer, buffersIntermediate[Buffer::eTraversalCounters]);
    lime::compute::pBarrierTransferWrite(commandBuffer);

    timestamps.Reset(commandBuffer);
    timestamps.Begin(commandBuffer);
    collapse(commandBuffer, inputBvh, geometryDescriptor);
    timestamps.End(commandBuffer);

    lime::compute::pBarrierTransferRead(commandBuffer);
    commandBuffer.copyBuffer(buffersOut[Buffer::eCollapsedNodeCounts].get(), stagingBuffer.get(), vk::BufferCopy(0, 0, 8));
}

void Collapsing::ReadRuntimeData()
{
    if (config.maxLeafSize > 1) {
        metadata.nodeCountLeaf = static_cast<u32*>(stagingBuffer.getMapping())[1];
        metadata.nodeCountTotal = static_cast<u32*>(stagingBuffer.getMapping())[0] + metadata.nodeCountLeaf + 1;
    }
}

stats::Collapsing Collapsing::GatherStats(BvhStats const& bvhStats)
{
    stats::Collapsing stats;
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

void Collapsing::reloadPipelines()
{
    metadata.workgroupSize = CreateSpecializationConstants(ctx.memory.pd);

    static std::array<vk::SpecializationMapEntry, 1> constexpr entries {
        vk::SpecializationMapEntry { 0, 0, sizeof(u32) },
    };
    vk::SpecializationInfo sInfo { 1, entries.data(), 4, &metadata.workgroupSize };

    pCollapse = { ctx.d, ctx.sCache, config.shader.collapse, sInfo };
}

void Collapsing::collapse(vk::CommandBuffer commandBuffer, Bvh const& inputBvh, vk::DeviceAddress geometryDescriptor)
{
    vk::MemoryBarrier memoryBarrierCompute { .srcAccessMask = vk::AccessFlagBits::eShaderWrite, .dstAccessMask = vk::AccessFlagBits::eShaderRead };
    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags(), memoryBarrierCompute, nullptr, nullptr);

    data_plocpp::PC_Collapse pc {
        .bvhAddress = inputBvh.bvh,
        .bvhTrianglesAddress = inputBvh.triangles,
        .bvhTriangleIndicesAddress = inputBvh.triangleIDs,
        .bvhCollapsedAddress = buffersOut[Buffer::eBVH].getDeviceAddress(ctx.d),
        .bvhCollapsedTrianglesAddress = buffersOut[Buffer::eBVHTriangles].getDeviceAddress(ctx.d),
        .bvhCollapsedTriangleIndicesAddress = buffersOut[Buffer::eBVHTriangleIDs].getDeviceAddress(ctx.d),

        .counterAddress = buffersIntermediate[Buffer::eTraversalCounters].getDeviceAddress(ctx.d),
        .nodeStateAddress = buffersIntermediate[Buffer::eNodeState].getDeviceAddress(ctx.d),
        .sahCostAddress = buffersIntermediate[Buffer::eSAHCost].getDeviceAddress(ctx.d),
        .leafNodeAddress = buffersIntermediate[Buffer::eLeafNodes].getDeviceAddress(ctx.d),
        .newNodeIdAddress = buffersIntermediate[Buffer::eNewNodeId].getDeviceAddress(ctx.d),
        .newTriIdAddress = buffersIntermediate[Buffer::eNewTriId].getDeviceAddress(ctx.d),
        .collapsedTreeNodeCountsAddress = buffersOut[Buffer::eCollapsedNodeCounts].getDeviceAddress(ctx.d),

        .schedulerDataAddress = buffersIntermediate[Buffer::eScheduler].getDeviceAddress(ctx.d),
        // .indirectDispatchBufferAddress = buffers[BufName::eIndirectDispatchBuffer].getDeviceAddress(ctx.d),
        .indirectDispatchBufferAddress = 0,

        .debugAddress = geometryDescriptor,

        .leafNodeCount = inputBvh.nodeCountLeaf,
        .maxLeafSize = config.maxLeafSize,

        .c_t = config.c_t,
        .c_i = config.c_i,
    };

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pCollapse.get());
    commandBuffer.pushConstants(pCollapse.layout.pipeline.get(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(pc), &pc);
    commandBuffer.dispatch(lime::divCeil(inputBvh.nodeCountLeaf, metadata.workgroupSize), 1, 1);
}

void Collapsing::freeIntermediate()
{
    buffersIntermediate.clear();
    stagingBuffer.reset();
}

void Collapsing::freeAllButGeometry()
{
    freeIntermediate();
    buffersOut.erase(Buffer::eBVH);
}

void Collapsing::freeAll()
{
    freeIntermediate();
    buffersOut.clear();
}

void Collapsing::alloc()
{
    freeAll();
    ctx.memory.cleanUp();

    auto const getNodeSize { [](config::BV bv) -> vk::DeviceSize {
        switch (bv) {
        case config::BV::eAABB:
            return sizeof(data_bvh::NodeBVH2_AABB);
        case config::BV::eDOP14:
            return sizeof(data_bvh::NodeBVH2_DOP14);
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
    buffersOut[Buffer::eBVH] = ctx.memory.alloc(aReq, cInfo, "bvh_collapsed");
    cInfo.size = sizeof(f32) * 12 * metadata.nodeCountLeaf;
    buffersOut[Buffer::eBVHTriangles] = ctx.memory.alloc(aReq, cInfo, "bvh_collapsed_triangles");
    cInfo.size = sizeof(u32) * 2 * metadata.nodeCountLeaf;
    buffersOut[Buffer::eBVHTriangleIDs] = ctx.memory.alloc(aReq, cInfo, "bvh_collapsed_triangle_ids");

    cInfo.size = sizeof(u32) * 5;
    buffersIntermediate[Buffer::eScheduler] = ctx.memory.alloc(aReq, cInfo, "collapsing_scheduler");

    cInfo.size = sizeof(u32) * metadata.nodeCountTotal;
    buffersIntermediate[Buffer::eTraversalCounters] = ctx.memory.alloc(aReq, cInfo, "collapsing_traversal_counters");
    buffersIntermediate[Buffer::eNodeState] = ctx.memory.alloc(aReq, cInfo, "collapsing_node_state");
    buffersIntermediate[Buffer::eSAHCost] = ctx.memory.alloc(aReq, cInfo, "collapsing_sah_cost");
    buffersIntermediate[Buffer::eLeafNodes] = ctx.memory.alloc(aReq, cInfo, "collapsing_leaf_nodes");
    buffersIntermediate[Buffer::eNewNodeId] = ctx.memory.alloc(aReq, cInfo, "collapsing_new_node_id");
    buffersIntermediate[Buffer::eNewTriId] = ctx.memory.alloc(aReq, cInfo, "collapsing_new_tri_id");

    cInfo.size = sizeof(u32) * 2;
    buffersOut[Buffer::eCollapsedNodeCounts] = ctx.memory.alloc(aReq, cInfo, "collapsed_node_counts");

    stagingBuffer = ctx.memory.alloc({ .memoryUsage = lime::DeviceMemoryUsage::eDeviceToHost }, { .size = 8, .usage = vk::BufferUsageFlagBits::eTransferDst }, "collapsing_staging");
}
}
