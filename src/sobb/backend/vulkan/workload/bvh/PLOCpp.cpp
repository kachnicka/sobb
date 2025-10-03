#include "PLOCpp.h"

#include "../../RadixSort.h"
#include "../../data/Scene.h"
#include <radix_sort/platforms/vk/radix_sort_vk.h>
#include <vLime/ComputeHelpers.h>

#include <final/shared/data_bvh.h>
#include <final/shared/data_plocpp.h>

namespace backend::vulkan::bvh {

static u32 SpecConstants(vk::PhysicalDevice pd)
{
    auto const prop2 { pd.getProperties2() };
    return prop2.properties.limits.maxComputeWorkGroupSize[0];
}

static data_plocpp::SC SpecConstantsPLOC(vk::PhysicalDevice pd, config::PLOC const& config)
{
    data_plocpp::SC result;

    auto const prop2 { pd.getProperties2<
        vk::PhysicalDeviceProperties2,
        vk::PhysicalDeviceSubgroupProperties,
        vk::PhysicalDeviceSubgroupSizeControlProperties>() };

    result.sizeWorkgroup = prop2.get<vk::PhysicalDeviceProperties2>().properties.limits.maxComputeWorkGroupSize[0];
    result.sizeSubgroup = prop2.get<vk::PhysicalDeviceSubgroupProperties>().subgroupSize;
    result.plocRadius = config.radius;

    // NOTE: createComputePipelineUnique() call now enforces 32, should reevaluate utility of this specConst
    result.sizeSubgroup = 32;

    auto totalSharedMemory = prop2.get<vk::PhysicalDeviceProperties2>().properties.limits.maxComputeSharedMemorySize;
    // fixed: prefix sum(2 * u32), scheduler(3 * u32)
    totalSharedMemory -= 5 * sizeof(u32);
    // size dependent: prefix sum cache (u32), bv cache (f * f32), nn id cache (u64)
    // eq, find max x, where x is subgroup/warp count to run per workgroup:
    //   sizeof(u32) * (x + x * warpSize * f + x * warpSize * 2) < totalSharedMemory
    u32 f = 6u;
    if (config.bv == config::BV::eDOP14)
        f = 14u;
    u32 const warpCount = totalSharedMemory / (sizeof(u32) * (1 + (f + 2) * result.sizeSubgroup));
    result.sizeWorkgroup = std::min(result.sizeWorkgroup, (warpCount << 5));

    return result;
}

PLOCpp::PLOCpp(VCtx ctx)
    : ctx(ctx)
    , timestamps(ctx.d, ctx.pd)
{
}

Bvh PLOCpp::GetBVH() const
{
    return {
        .bvh = buffersOut.at(Buffer::eBVH).getDeviceAddress(ctx.d),
        .triangles = buffersOut.contains(Buffer::eBVHTriangles)
            ? buffersOut.at(Buffer::eBVHTriangles).getDeviceAddress(ctx.d)
            : 0,
        .triangleIDs = buffersOut.at(Buffer::eBVHTriangleIDs).getDeviceAddress(ctx.d),
        .nodeCountLeaf = metadata.nodeCountLeaf,
        .nodeCountTotal = metadata.nodeCountTotal,
        .bv = config.bv,
        .layout = config::NodeLayout::eDefault,
    };
}

bool PLOCpp::CheckForShaderHotReload()
{
    if (config.bv == config::BV::eNone)
        return false;
    bool updated = false;
    for (auto& p : pipelines)
        updated = p.Update(ctx.d, ctx.sCache) || updated;
    return updated;
}

void PLOCpp::Compute(vk::CommandBuffer commandBuffer, data::Scene const& scene)
{
    metadata.nodeCountLeaf = scene.totalTriangleCount;
    metadata.nodeCountTotal = scene.totalTriangleCount * 2 - 1;
    metadata.sceneMeshCount_TMP = csize<u32>(scene.geometries);

    reloadPipelines();
    alloc();
    lime::compute::fillZeros(commandBuffer, buffersIntermediate[Buffer::eRuntimeData]);

    lime::compute::fillZeros(commandBuffer, buffersOut[Buffer::eBVH]);

    // init input for indirect dispatch buffer fill kernel
    commandBuffer.fillBuffer(buffersIntermediate[Buffer::eIndirectDispatchBuffer].get(), 0, 4, metadata.nodeCountLeaf);
    commandBuffer.fillBuffer(buffersIntermediate[Buffer::eIndirectDispatchBuffer].get(), 4, 4, metadata.nodeCountLeaf);
    commandBuffer.fillBuffer(buffersIntermediate[Buffer::eIndirectDispatchBuffer].get(), 8, 20, 0);

    timestamps.Reset(commandBuffer);
    timestamps.WriteBeginStamp(commandBuffer);

    initialClusters(commandBuffer, scene);
    timestamps.Write(commandBuffer, Times::Stamp::eInitialClustersAndWoopify);

    // wait for init writes
    lime::compute::pBarrierTransferWrite(commandBuffer);
    lime::compute::pBarrierCompute(commandBuffer);

    // fill indirect dispatch buffer
    auto idbAddr { buffersIntermediate[Buffer::eIndirectDispatchBuffer].getDeviceAddress(ctx.d) };
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipelines[Pipeline::eFillIndirect].get());
    commandBuffer.pushConstants(pipelines[Pipeline::eFillIndirect].layout.pipeline.get(), vk::ShaderStageFlagBits::eCompute, 0, 8, &idbAddr);
    commandBuffer.dispatch(1, 1, 1);

    sortClusterIDs(commandBuffer);
    timestamps.Write(commandBuffer, Times::Stamp::eSortClusterIDs);

    copySortedClusterIDs(commandBuffer);
    timestamps.Write(commandBuffer, Times::Stamp::eCopySortedClusterIDs);

    {
        lime::compute::pBarrierTransferRead(commandBuffer);
        commandBuffer.fillBuffer(buffersIntermediate[Buffer::eRuntimeData].get(), 0, 40, 0);
        lime::compute::pBarrierTransferWrite(commandBuffer);
        iterationsSingleKernel(commandBuffer);
    }
    timestamps.Write(commandBuffer, Times::Stamp::ePLOCppIterations);

    lime::compute::pBarrierTransferRead(commandBuffer);
    commandBuffer.copyBuffer(buffersIntermediate[Buffer::eRuntimeData].get(), stagingBuffer.get(), vk::BufferCopy(36, 0, 4));
}

void PLOCpp::ReadRuntimeData()
{
    metadata.iterationCount = *static_cast<u32*>(stagingBuffer.getMapping());

    // NOTE: works only with resizable bar, should rework whole pipeline to indirect dispatch
    metadata.nodeCountLeaf = *static_cast<u32*>(buffersIntermediate[Buffer::eIndirectDispatchBuffer].getMapping());
    metadata.nodeCountTotal = metadata.nodeCountLeaf * 2 - 1;
}

stats::PLOC PLOCpp::GatherStats(BvhStats const& bvhStats)
{
    stats::PLOC stats;

    stats.times = timestamps.GetResultsNs();
    for (auto& t : stats.times) {
        t *= 1e-6f;
        stats.timeTotal += t;
    }

    stats.iterationCount = metadata.iterationCount;

    stats.saIntersect = bvhStats.saIntersect;
    stats.saTraverse = bvhStats.saTraverse;
    stats.costTotal = bvhStats.costIntersect + bvhStats.costTraverse;

    stats.nodeCountTotal = metadata.nodeCountTotal;
    stats.leafSizeMin = bvhStats.leafSizeMin;
    stats.leafSizeMax = bvhStats.leafSizeMax;
    stats.leafSizeAvg = static_cast<f32>(bvhStats.leafSizeSum) / static_cast<f32>(metadata.nodeCountLeaf);

    return stats;
}

void PLOCpp::reloadPipelines()
{
    auto const sc = SpecConstantsPLOC(ctx.pd, config);
    metadata.workgroupSize = SpecConstants(ctx.pd);
    metadata.workgroupSizePLOCpp = sc.sizeWorkgroup;

    static std::array<vk::SpecializationMapEntry, 3> constexpr entries {
        vk::SpecializationMapEntry { 0, static_cast<u32>(offsetof(data_plocpp::SC, sizeWorkgroup)), sizeof(u32) },
        vk::SpecializationMapEntry { 1, static_cast<u32>(offsetof(data_plocpp::SC, sizeSubgroup)), sizeof(u32) },
        vk::SpecializationMapEntry { 2, static_cast<u32>(offsetof(data_plocpp::SC, plocRadius)), sizeof(u32) },
    };
    vk::SpecializationInfo sInfo { 1, entries.data(), 4, &metadata.workgroupSize };
    vk::SpecializationInfo sInfoPLOC { 3, entries.data(), 12, &sc };

    pipelines[Pipeline::eFillIndirect] = { ctx.d, ctx.sCache, "final/plocpp_FillIndirect.comp.spv" };
    pipelines[Pipeline::eCopySortedClusterIDs] = { ctx.d, ctx.sCache, "final/plocpp_CopyClusterIDs.comp.spv", sInfo };
    pipelines[Pipeline::eInitialClusters] = { ctx.d, ctx.sCache, config.shader.initialClusters, sInfo };
    pipelines[Pipeline::ePLOCppIterations] = { ctx.d, ctx.sCache, config.shader.iterations, sInfoPLOC };
}

void PLOCpp::initialClusters(vk::CommandBuffer commandBuffer, data::Scene const& scene)
{
    auto const cubedAabb { scene.aabb.GetCubed() };
    data_plocpp::PC_MortonGlobal pcGlobal {
        .sceneAabbCubedMin = { cubedAabb.min.x, cubedAabb.min.y, cubedAabb.min.z },
        .sceneAabbNormalizationScale = 1.f / (cubedAabb.max - cubedAabb.min).x,
        .mortonAddress = buffersIntermediate[Buffer::eRadixEven].getDeviceAddress(ctx.d),
        .bvhAddress = buffersOut[Buffer::eBVH].getDeviceAddress(ctx.d),

        .bvhTrianglesAddress = buffersOut[Buffer::eBVHTriangles].getDeviceAddress(ctx.d),
        .bvhTriangleIndicesAddress = buffersOut[Buffer::eBVHTriangleIDs].getDeviceAddress(ctx.d),
        .auxBufferAddress = buffersIntermediate[Buffer::eIndirectDispatchBuffer].getDeviceAddress(ctx.d),
    };

    data_plocpp::PC_MortonPerGeometry pcPerGeometry {
        .idxAddress = 0,
        .vtxAddress = 0,
        .globalTriangleIdBase = 0,
        .sceneNodeId = 0,
        .triangleCount = 0,
    };

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipelines[Pipeline::eInitialClusters].get());
    commandBuffer.pushConstants(
        pipelines[Pipeline::eInitialClusters].layout.pipeline.get(),
        vk::ShaderStageFlagBits::eCompute,
        0, sizeof(pcGlobal), &pcGlobal);

    for (auto const& gId : scene.geometries) {
        auto const& g { scene.data->geometries[gId] };

        pcPerGeometry.idxAddress = g.indexBuffer.getDeviceAddress(ctx.d);
        pcPerGeometry.vtxAddress = g.vertexBuffer.getDeviceAddress(ctx.d);
        pcPerGeometry.sceneNodeId = gId.get();
        pcPerGeometry.triangleCount = g.indexCount / 3;

        commandBuffer.pushConstants(
            pipelines[Pipeline::eInitialClusters].layout.pipeline.get(),
            vk::ShaderStageFlagBits::eCompute,
            sizeof(pcGlobal), sizeof(pcPerGeometry), &pcPerGeometry);
        commandBuffer.dispatch(lime::divCeil(pcPerGeometry.triangleCount, metadata.workgroupSize), 1, 1);

        pcPerGeometry.globalTriangleIdBase += pcPerGeometry.triangleCount;
    }
}

void PLOCpp::sortClusterIDs(vk::CommandBuffer commandBuffer)
{
    vk::MemoryBarrier memoryBarrierCompute { .srcAccessMask = vk::AccessFlagBits::eShaderWrite, .dstAccessMask = vk::AccessFlagBits::eShaderRead };
    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags(), memoryBarrierCompute, nullptr, nullptr);

    radix_sort_vk_memory_requirements_t radixSortMemory;
    radix_sort_vk_get_memory_requirements(FuchsiaRadixSort::radixSort, metadata.nodeCountLeaf, &radixSortMemory);
    VkDescriptorBufferInfo results;

    radix_sort_vk_sort_indirect_info_t radixInfoIndirect;
    radixInfoIndirect.ext = nullptr;
    radixInfoIndirect.key_bits = 32;
    radixInfoIndirect.count = { buffersIntermediate[Buffer::eIndirectDispatchBuffer].get(), 0, 4 };
    radixInfoIndirect.keyvals_even = { buffersIntermediate[Buffer::eRadixEven].get(), 0, radixSortMemory.keyvals_size };
    radixInfoIndirect.keyvals_odd = { buffersIntermediate[Buffer::eRadixOdd].get(), 0, radixSortMemory.keyvals_size };
    radixInfoIndirect.internal = { buffersIntermediate[Buffer::eRadixInternal].get(), 0, radixSortMemory.internal_size };
    radixInfoIndirect.indirect = { buffersIntermediate[Buffer::eRadixIndirect].get(), 0, radixSortMemory.indirect_size };
    radix_sort_vk_sort_indirect(FuchsiaRadixSort::radixSort, &radixInfoIndirect, ctx.d, commandBuffer, &results);

    auto& even { buffersIntermediate[Buffer::eRadixEven] };
    auto& odd { buffersIntermediate[Buffer::eRadixOdd] };
    nodeBuffer1Address = (vk::Buffer(results.buffer) == even.get() ? even : odd).getDeviceAddress(ctx.d);
    nodeBuffer0Address = (vk::Buffer(results.buffer) == even.get() ? odd : even).getDeviceAddress(ctx.d);
}

void PLOCpp::copySortedClusterIDs(vk::CommandBuffer commandBuffer)
{
    vk::MemoryBarrier memoryBarrierCompute { .srcAccessMask = vk::AccessFlagBits::eShaderWrite, .dstAccessMask = vk::AccessFlagBits::eShaderRead };
    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags(), memoryBarrierCompute, nullptr, nullptr);

    data_plocpp::PC_CopySortedNodeIds pc {
        .mortonAddress = nodeBuffer1Address,
        .nodeIdAddress = nodeBuffer0Address,
        .clusterCount = metadata.nodeCountLeaf,
    };

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipelines[Pipeline::eCopySortedClusterIDs].get());
    commandBuffer.pushConstants(pipelines[Pipeline::eCopySortedClusterIDs].layout.pipeline.get(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(pc), &pc);

    // wait for indirect dispatch buffer, written in previous stage
    vk::MemoryBarrier indirectDispatchBarrierAccess { .srcAccessMask = vk::AccessFlagBits::eShaderWrite, .dstAccessMask = vk::AccessFlagBits::eIndirectCommandRead };
    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eAllCommands, vk::DependencyFlags(), indirectDispatchBarrierAccess, nullptr, nullptr);
    commandBuffer.dispatchIndirect(buffersIntermediate[Buffer::eIndirectDispatchBuffer].get(), 16);
}

void PLOCpp::iterationsSingleKernel(vk::CommandBuffer commandBuffer)
{
    vk::MemoryBarrier memoryBarrierCompute { .srcAccessMask = vk::AccessFlagBits::eShaderWrite, .dstAccessMask = vk::AccessFlagBits::eShaderRead };
    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags(), memoryBarrierCompute, nullptr, nullptr);

    data_plocpp::PC_PlocppIterationIndirect pc {
        .bvhAddress = buffersOut[Buffer::eBVH].getDeviceAddress(ctx.d),
        .nodeId0Address = nodeBuffer0Address,
        .nodeId1Address = nodeBuffer1Address,
        .dlWorkBufAddress = buffersIntermediate[Buffer::eDecoupledLookBack].getDeviceAddress(ctx.d),
        .runtimeDataAddress = buffersIntermediate[Buffer::eRuntimeData].getDeviceAddress(ctx.d),

        .auxBufferAddress = buffersIntermediate[Buffer::eDebug].getDeviceAddress(ctx.d),
        .idbAddress = buffersIntermediate[Buffer::eIndirectDispatchBuffer].getDeviceAddress(ctx.d),
    };

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipelines[Pipeline::ePLOCppIterations].get());
    commandBuffer.pushConstants(
        pipelines[Pipeline::ePLOCppIterations].layout.pipeline.get(),
        vk::ShaderStageFlagBits::eCompute,
        0, sizeof(pc), &pc);

    commandBuffer.dispatch(1024, 1, 1);
}

void PLOCpp::pairDiscovery(vk::CommandBuffer commandBuffer, data::Scene const& scene)
{
    buffersOut[Buffer::eBVH].CopyMeToBuffer(commandBuffer, buffersIntermediate[Buffer::eScratchBVHNodes], 0);
    buffersOut[Buffer::eBVHTriangleIDs].CopyMeToBuffer(commandBuffer, buffersIntermediate[Buffer::eScratchTriIds], 0);
    if (nodeBuffer0Address == buffersIntermediate[Buffer::eRadixEven].getDeviceAddress(ctx.d))
        buffersIntermediate[Buffer::eRadixEven].CopyMeToBuffer(commandBuffer, buffersIntermediate[Buffer::eScratchIds0], 0);
    else
        buffersIntermediate[Buffer::eRadixOdd].CopyMeToBuffer(commandBuffer, buffersIntermediate[Buffer::eScratchIds0], 0);
    lime::compute::pBarrierTransferWrite(commandBuffer);
    lime::compute::pBarrierCompute(commandBuffer);

    data_plocpp::PC_DiscoverPairs pc {
        .bvhAddress = buffersOut[Buffer::eBVH].getDeviceAddress(ctx.d),
        .nodeId0Address = nodeBuffer0Address,
        .nodeId1Address = nodeBuffer1Address,
        .dlWorkBufAddress = buffersIntermediate[Buffer::eDecoupledLookBack].getDeviceAddress(ctx.d),
        .runtimeDataAddress = buffersIntermediate[Buffer::eRuntimeData].getDeviceAddress(ctx.d),
        .idbAddress = buffersIntermediate[Buffer::eIndirectDispatchBuffer].getDeviceAddress(ctx.d),

        .scratchNodesAddress = buffersIntermediate[Buffer::eScratchBVHNodes].getDeviceAddress(ctx.d),
        .scratchIds0Address = buffersIntermediate[Buffer::eScratchIds0].getDeviceAddress(ctx.d),
        .scratchIds1Address = buffersIntermediate[Buffer::eScratchIds1].getDeviceAddress(ctx.d),
        .scratchTriIdsAddress = buffersIntermediate[Buffer::eScratchTriIds].getDeviceAddress(ctx.d),

        .bvhTriangleIndicesAddress = buffersOut[Buffer::eBVHTriangleIDs].getDeviceAddress(ctx.d),
        .geometryDescriptorAddress = scene.data->sceneDescriptionBuffer.getDeviceAddress(ctx.d),
    };

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipelines[Pipeline::ePairDiscovery].get());
    commandBuffer.pushConstants(
        pipelines[Pipeline::ePairDiscovery].layout.pipeline.get(),
        vk::ShaderStageFlagBits::eCompute,
        0, sizeof(pc), &pc);

    commandBuffer.dispatch(1024, 1, 1);
}

void PLOCpp::freeIntermediate()
{
    buffersIntermediate.clear();
    nodeBuffer0Address = 0;
    nodeBuffer1Address = 0;
    stagingBuffer.reset();
}

void PLOCpp::freeAll()
{
    freeIntermediate();
    buffersOut.clear();
}

void PLOCpp::alloc()
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
    buffersOut[Buffer::eBVH] = ctx.memory.alloc(aReq, cInfo, "bvh_plocpp");
    cInfo.size = sizeof(u32) * 2 * metadata.nodeCountLeaf;
    buffersOut[Buffer::eBVHTriangleIDs] = ctx.memory.alloc(aReq, cInfo, "bvh_plocpp_triangle_ids");

    switch (config.bv) {
    case config::BV::eNone:
        [[fallthrough]];
    default:
        cInfo.size = sizeof(f32) * 12 * metadata.nodeCountLeaf;
        buffersOut[Buffer::eBVHTriangles] = ctx.memory.alloc(aReq, cInfo, "bvh_plocpp_triangles");
        break;
    }

    radix_sort_vk_memory_requirements_t radixSortMemory;
    radix_sort_vk_get_memory_requirements(FuchsiaRadixSort::radixSort, metadata.nodeCountLeaf, &radixSortMemory);
    cInfo.size = radixSortMemory.keyvals_size;
    aReq.additionalAlignment = radixSortMemory.keyvals_alignment;
    buffersIntermediate[Buffer::eRadixEven] = ctx.memory.alloc(aReq, cInfo, "plocpp_radix_even");
    buffersIntermediate[Buffer::eRadixOdd] = ctx.memory.alloc(aReq, cInfo, "plocpp_radix_odd");
    cInfo.size = radixSortMemory.internal_size;
    aReq.additionalAlignment = radixSortMemory.internal_alignment;
    buffersIntermediate[Buffer::eRadixInternal] = ctx.memory.alloc(aReq, cInfo, "plocpp_radix_internal");

    aReq.additionalAlignment = 256;
    cInfo.size = sizeof(u32) * 10;
    buffersIntermediate[Buffer::eRuntimeData] = ctx.memory.alloc(aReq, cInfo, "plocpp_runtime_data");

    cInfo.size = sizeof(u32) * 2 * lime::divCeil(metadata.nodeCountLeaf, metadata.workgroupSizePLOCpp - 4 * config.radius);
    buffersIntermediate[Buffer::eDecoupledLookBack] = ctx.memory.alloc(aReq, cInfo, "plocpp_decoupled_lookback");

    cInfo.size = sizeof(f32) * metadata.nodeCountTotal * 2;
    buffersIntermediate[Buffer::eDebug] = ctx.memory.alloc(aReq, cInfo, "plocpp_debug");

    cInfo.size = sizeof(data_plocpp::IndirectClusters);
    cInfo.usage |= bfub::eIndirectBuffer;
    buffersIntermediate[Buffer::eIndirectDispatchBuffer] = ctx.memory.alloc(aReq, cInfo, "plocpp_indirect_clusters");
    cInfo.size = radixSortMemory.indirect_size;
    aReq.additionalAlignment = radixSortMemory.indirect_alignment;
    buffersIntermediate[Buffer::eRadixIndirect] = ctx.memory.alloc(aReq, cInfo, "plocpp_radix_idirect");

    stagingBuffer = ctx.memory.alloc({ .memoryUsage = lime::DeviceMemoryUsage::eDeviceToHost }, { .size = 4, .usage = vk::BufferUsageFlagBits::eTransferDst }, "plocpp_staging");
}

}
