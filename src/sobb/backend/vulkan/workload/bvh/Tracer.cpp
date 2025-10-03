#include "Tracer.h"
#include "vLime/ComputeHelpers.h"

#include <final/shared/data_ptrace.h>

namespace backend::vulkan::bvh {
using bfub = vk::BufferUsageFlagBits;

static u32 CreateSpecializationConstants(vk::PhysicalDevice pd)
{
    auto const prop2 { pd.getProperties2() };
    return prop2.properties.limits.maxComputeWorkGroupSize[0];
}

Tracer::Tracer(VCtx ctx)
    : ctx(ctx)
    , timestamp(ctx.d, ctx.pd)
{
    allocStatic();

    std::vector<vk::DescriptorPoolSize> poolSizes {
        { vk::DescriptorType::eStorageImage, 1 },
        { vk::DescriptorType::eUniformBufferDynamic, 1 },
    };
    vk::DescriptorPoolCreateInfo cInfo;
    cInfo.maxSets = 1;
    cInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    cInfo.pPoolSizes = poolSizes.data();
    dPool = lime::check(ctx.d.createDescriptorPoolUnique(cInfo));
}

void Tracer::Trace(vk::CommandBuffer commandBuffer, config::Tracer const& traceCfg, TraceRuntime const& trt, Bvh const& inputBvh)
{
    if (traceCfg != config) {
        config = traceCfg;
        metadata.reloadPipelines = true;
    }
    if (inputBvh.layout != metadata.bvhMemoryLayout) {
        metadata.bvhMemoryLayout = inputBvh.layout;
        metadata.reloadPipelines = true;
    }
    if (config.bv == config::BV::eNone)
        return;

    if (metadata.reloadPipelines) {
        reloadPipelines();
        metadata.reloadPipelines = false;
    } else
        checkForShaderHotReload();

    if (u32 rayCount { trt.x * trt.y }; rayCount > metadata.rayCount || metadata.reallocRayBuffers) {
        freeRayBuffers();
        allocRayBuffers(rayCount);
        metadata.reallocRayBuffers = false;
    }
    dSetUpdate(trt.targetImageView, trt.camera);

    lime::debug::BeginDebugLabel(commandBuffer, "path tracing compute", lime::debug::LabelColor::eBordeaux);

    switch (metadata.visMode) {
    case State::VisMode::ePathTracing:
        trace_separate(commandBuffer, trt, inputBvh);
        break;
    case State::VisMode::eBVVisualization:
        trace_separate_bv(commandBuffer, trt, inputBvh);
        break;
    case State::VisMode::eBVIntersections:
        trace_separate(commandBuffer, trt, inputBvh);
        break;
    default:
        break;
    }
    lime::debug::EndDebugLabel(commandBuffer);
}

void Tracer::trace_separate(vk::CommandBuffer commandBuffer, TraceRuntime const& trt, Bvh const& inputBvh)
{
    data_ptrace::PC_GenPrimary pcGenPrimary {
        .rayBufferMetadataAddress = bRayMetadata[0].getDeviceAddress(ctx.d),
        .rayBufferAddress = bRay[0].getDeviceAddress(ctx.d),
        .rayPayloadAddress = bRayPayload[0].getDeviceAddress(ctx.d),
        .samplesComputed = trt.samples.computed,
        .samplesToComputeThisFrame = trt.samples.toCompute,
    };

    data_ptrace::PC_Trace pcTrace {
        .rayBufferMetadataAddress = bRayMetadata[0].getDeviceAddress(ctx.d),
        .rayBufferAddress = bRay[0].getDeviceAddress(ctx.d),
        .rayTraceResultAddress = bTraceResult.getDeviceAddress(ctx.d),

        .bvhAddress = inputBvh.bvh,
        .bvhTrianglesAddress = inputBvh.triangles,
        .bvhTriangleIndicesAddress = inputBvh.triangleIDs,
        .bvhAuxAddress = inputBvh.bvhAux,

        .traceTimeAddress = bTimes.getDeviceAddress(ctx.d),
        .traceStatsAddress = bStats.getDeviceAddress(ctx.d),
    };

    data_ptrace::PC_ShadeCast pcShadeCast {
        .dirLight = { dirLight[0], dirLight[1], dirLight[2], dirLight[3] },

        .read_rayBufferMetadataAddress = bRayMetadata[0].getDeviceAddress(ctx.d),
        .read_rayBufferAddress = bRay[0].getDeviceAddress(ctx.d),
        .read_rayPayloadAddress = bRayPayload[0].getDeviceAddress(ctx.d),

        .write_rayBufferMetadataAddress = bRayMetadata[1].getDeviceAddress(ctx.d),
        .write_rayBufferAddress = bRay[1].getDeviceAddress(ctx.d),
        .write_rayPayloadAddress = bRayPayload[1].getDeviceAddress(ctx.d),

        .rayTraceResultAddress = bTraceResult.getDeviceAddress(ctx.d),
        .geometryDescriptorAddress = trt.geometryDescriptorAddress,
        .bvhTriangleIndicesAddress = inputBvh.triangleIDs,

        .depth = 0,
        .depthMax = trt.ptDepth,
        .samplesComputed = trt.samples.computed,
    };

    auto const setupPCForDepth { [this, &pcTrace, &pcShadeCast](u32 depth) -> u32 {
        u32 const idRead { depth % 2 };
        u32 const idWrite { (depth + 1) % 2 };

        pcTrace.rayBufferMetadataAddress = bRayMetadata[idRead].getDeviceAddress(ctx.d);
        pcTrace.rayBufferAddress = bRay[idRead].getDeviceAddress(ctx.d);

        pcTrace.traceTimeAddress = bTimes.getDeviceAddress(ctx.d) + 16 * depth;

        pcShadeCast.read_rayBufferMetadataAddress = bRayMetadata[idRead].getDeviceAddress(ctx.d);
        pcShadeCast.read_rayBufferAddress = bRay[idRead].getDeviceAddress(ctx.d);
        pcShadeCast.read_rayPayloadAddress = bRayPayload[idRead].getDeviceAddress(ctx.d);
        pcShadeCast.write_rayBufferMetadataAddress = bRayMetadata[idWrite].getDeviceAddress(ctx.d);
        pcShadeCast.write_rayBufferAddress = bRay[idWrite].getDeviceAddress(ctx.d);
        pcShadeCast.write_rayPayloadAddress = bRayPayload[idWrite].getDeviceAddress(ctx.d);
        pcShadeCast.depth = depth;

        // necessary to clear metadata at this id
        return idWrite;
    } };

    std::vector<vk::DescriptorSet> desc_set { dSet };

    // init buffers
    lime::compute::fillZeros(commandBuffer, bStats);
    lime::compute::fillZeros(commandBuffer, bRayMetadata[0]);
    lime::compute::fillZeros(commandBuffer, bRayMetadata[1]);
    for (u32 depth = 0; depth < trt.ptDepth + 1; depth++) {
        commandBuffer.fillBuffer(bTimes.get(), 0 + 16 * depth, 8, uint32_t(-1));
        commandBuffer.fillBuffer(bTimes.get(), 8 + 16 * depth, 8, 0);
    }
    lime::compute::pBarrierTransferWrite(commandBuffer);

    timestamp.Reset(commandBuffer);
    timestamp.Begin(commandBuffer);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipelines[Pipeline::eGenPrimary].get());
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelines[Pipeline::eGenPrimary].getLayout(), 0, desc_set, { 0 });
    commandBuffer.pushConstants(pipelines[Pipeline::eGenPrimary].getLayout(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(pcGenPrimary), &pcGenPrimary);
    commandBuffer.dispatch(lime::divCeil(trt.x, metadata.primaryRaysTileSize.x), lime::divCeil(trt.y, metadata.primaryRaysTileSize.y), 1);

    for (u32 depth = 0; depth < trt.ptDepth + 1; depth++) {
        auto const clearId = setupPCForDepth(depth);

        u32 traceWorkgroupCount = 0;
        // primary rays
        if (depth == 0) {
            commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipelines[Pipeline::eTracePrimary].get());
            commandBuffer.pushConstants(pipelines[Pipeline::eTracePrimary].getLayout(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(pcTrace), &pcTrace);
            traceWorkgroupCount = config.rPrimary.workgroupCount;
        }
        // secondary rays
        else {
            commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipelines[Pipeline::eTraceSecondary].get());
            commandBuffer.pushConstants(pipelines[Pipeline::eTraceSecondary].getLayout(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(pcTrace), &pcTrace);
            traceWorkgroupCount = config.rSecondary.workgroupCount;
        }

        lime::compute::pBarrierCompute(commandBuffer);
        commandBuffer.dispatch(traceWorkgroupCount, 1, 1);

        commandBuffer.fillBuffer(bRayMetadata[clearId].get(), 0, bRayMetadata[clearId].getSizeInBytes(), 0);
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipelines[Pipeline::eShadeAndCast].get());
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelines[Pipeline::eShadeAndCast].getLayout(), 0, desc_set, { 0 });
        commandBuffer.pushConstants(pipelines[Pipeline::eShadeAndCast].getLayout(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(pcShadeCast), &pcShadeCast);

        // wait for trace
        lime::compute::pBarrierCompute(commandBuffer);
        // wait for metadata clear
        lime::compute::pBarrierTransferWrite(commandBuffer);
        commandBuffer.dispatch(lime::divCeil(metadata.rayCount, 1024u), 1, 1);
    }

    timestamp.End(commandBuffer);

    lime::compute::pBarrierTransferRead(commandBuffer);
    commandBuffer.copyBuffer(bTimes.get(), bStaging.get(), vk::BufferCopy(0, 0, bTimes.getSizeInBytes()));
}

void Tracer::trace_separate_bv(vk::CommandBuffer commandBuffer, TraceRuntime const& trt, Bvh const& inputBvh)
{
    data_ptrace::PC_GenPrimary pcGenPrimary {
        .rayBufferMetadataAddress = bRayMetadata[0].getDeviceAddress(ctx.d),
        .rayBufferAddress = bRay[0].getDeviceAddress(ctx.d),
        .rayPayloadAddress = bRayPayload[0].getDeviceAddress(ctx.d),
        .samplesComputed = trt.samples.computed,
        .samplesToComputeThisFrame = trt.samples.toCompute,
    };

    data_ptrace::PC_Trace pcTrace {
        .rayBufferMetadataAddress = bRayMetadata[0].getDeviceAddress(ctx.d),
        .rayBufferAddress = bRay[0].getDeviceAddress(ctx.d),
        .rayTraceResultAddress = bTraceResult.getDeviceAddress(ctx.d),

        .bvhAddress = inputBvh.bvh,
        .bvhTrianglesAddress = inputBvh.triangles,
        .bvhTriangleIndicesAddress = inputBvh.triangleIDs,
        .bvhAuxAddress = inputBvh.bvhAux,

        .traceTimeAddress = bTimes.getDeviceAddress(ctx.d),
        .traceStatsAddress = bStats.getDeviceAddress(ctx.d),
    };

    data_ptrace::PC_ShadeCast pcShadeCast {
        .dirLight = { dirLight[0], dirLight[1], dirLight[2], dirLight[3] },

        .read_rayBufferMetadataAddress = bRayMetadata[0].getDeviceAddress(ctx.d),
        .read_rayBufferAddress = bRay[0].getDeviceAddress(ctx.d),
        .read_rayPayloadAddress = bRayPayload[0].getDeviceAddress(ctx.d),

        .write_rayBufferMetadataAddress = bRayMetadata[1].getDeviceAddress(ctx.d),
        .write_rayBufferAddress = bRay[1].getDeviceAddress(ctx.d),
        .write_rayPayloadAddress = bRayPayload[1].getDeviceAddress(ctx.d),

        .rayTraceResultAddress = bTraceResult.getDeviceAddress(ctx.d),
        .geometryDescriptorAddress = trt.geometryDescriptorAddress,
        .bvhTriangleIndicesAddress = inputBvh.triangleIDs,

        .depth = 0,
        .depthMax = trt.ptDepth,
        .samplesComputed = trt.samples.computed,
    };

    auto const setupPCForDepth { [this, &pcTrace, &pcShadeCast](u32 depth) -> u32 {
        u32 const idRead { depth % 2 };
        u32 const idWrite { (depth + 1) % 2 };

        pcTrace.rayBufferMetadataAddress = bRayMetadata[idRead].getDeviceAddress(ctx.d);
        pcTrace.rayBufferAddress = bRay[idRead].getDeviceAddress(ctx.d);

        pcTrace.traceTimeAddress = bTimes.getDeviceAddress(ctx.d) + 16 * depth;

        pcShadeCast.read_rayBufferMetadataAddress = bRayMetadata[idRead].getDeviceAddress(ctx.d);
        pcShadeCast.read_rayBufferAddress = bRay[idRead].getDeviceAddress(ctx.d);
        pcShadeCast.read_rayPayloadAddress = bRayPayload[idRead].getDeviceAddress(ctx.d);
        pcShadeCast.write_rayBufferMetadataAddress = bRayMetadata[idWrite].getDeviceAddress(ctx.d);
        pcShadeCast.write_rayBufferAddress = bRay[idWrite].getDeviceAddress(ctx.d);
        pcShadeCast.write_rayPayloadAddress = bRayPayload[idWrite].getDeviceAddress(ctx.d);
        pcShadeCast.depth = depth;

        // necessary to clear metadata at this id
        return idWrite;
    } };

    std::vector<vk::DescriptorSet> desc_set { dSet };

    // init buffers
    lime::compute::fillZeros(commandBuffer, bStats);
    lime::compute::fillZeros(commandBuffer, bRayMetadata[0]);
    lime::compute::fillZeros(commandBuffer, bRayMetadata[1]);
    for (u32 depth = 0; depth < trt.ptDepth + 1; depth++) {
        commandBuffer.fillBuffer(bTimes.get(), 0 + 16 * depth, 8, uint32_t(-1));
        commandBuffer.fillBuffer(bTimes.get(), 8 + 16 * depth, 8, 0);
    }
    lime::compute::pBarrierTransferWrite(commandBuffer);

    timestamp.Reset(commandBuffer);
    timestamp.Begin(commandBuffer);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipelines[Pipeline::eGenPrimary].get());
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelines[Pipeline::eGenPrimary].getLayout(), 0, desc_set, { 0 });
    commandBuffer.pushConstants(pipelines[Pipeline::eGenPrimary].getLayout(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(pcGenPrimary), &pcGenPrimary);
    commandBuffer.dispatch(lime::divCeil(trt.x, metadata.primaryRaysTileSize.x), lime::divCeil(trt.y, metadata.primaryRaysTileSize.y), 1);

    for (u32 depth = 0; depth < trt.ptDepth + 1; depth++) {
        auto const clearId = setupPCForDepth(depth);

        u32 traceWorkgroupCount = 0;
        // primary rays
        if (depth == 0) {
            commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipelines[Pipeline::eTracePrimary].get());
            commandBuffer.pushConstants(pipelines[Pipeline::eTracePrimary].getLayout(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(pcTrace), &pcTrace);
            if (config.bvRenderTriangles) {
                u32 i = 0;
                commandBuffer.pushConstants(pipelines[Pipeline::eTracePrimary].getLayout(), vk::ShaderStageFlagBits::eCompute, sizeof(pcTrace), 4, &i);
            } else
                commandBuffer.pushConstants(pipelines[Pipeline::eTracePrimary].getLayout(), vk::ShaderStageFlagBits::eCompute, sizeof(pcTrace), 4, &config.bvDepth);
            traceWorkgroupCount = config.rPrimary.workgroupCount;
        }
        // secondary rays
        else {
            commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipelines[Pipeline::eTraceSecondary].get());
            commandBuffer.pushConstants(pipelines[Pipeline::eTraceSecondary].getLayout(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(pcTrace), &pcTrace);
            if (config.bvRenderTriangles) {
                u32 i = 0;
                commandBuffer.pushConstants(pipelines[Pipeline::eTracePrimary].getLayout(), vk::ShaderStageFlagBits::eCompute, sizeof(pcTrace), 4, &i);
            } else
                commandBuffer.pushConstants(pipelines[Pipeline::eTracePrimary].getLayout(), vk::ShaderStageFlagBits::eCompute, sizeof(pcTrace), 4, &config.bvDepth);
            traceWorkgroupCount = config.rSecondary.workgroupCount;
        }

        lime::compute::pBarrierCompute(commandBuffer);
        commandBuffer.dispatch(traceWorkgroupCount, 1, 1);

        commandBuffer.fillBuffer(bRayMetadata[clearId].get(), 0, bRayMetadata[clearId].getSizeInBytes(), 0);
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipelines[Pipeline::eShadeAndCast].get());
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelines[Pipeline::eShadeAndCast].getLayout(), 0, desc_set, { 0 });
        commandBuffer.pushConstants(pipelines[Pipeline::eShadeAndCast].getLayout(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(pcShadeCast), &pcShadeCast);

        // wait for trace
        lime::compute::pBarrierCompute(commandBuffer);
        // wait for metadata clear
        lime::compute::pBarrierTransferWrite(commandBuffer);
        commandBuffer.dispatch(lime::divCeil(metadata.rayCount, 1024u), 1, 1);
    }

    timestamp.End(commandBuffer);

    lime::compute::pBarrierTransferRead(commandBuffer);
    commandBuffer.copyBuffer(bTimes.get(), bStaging.get(), vk::BufferCopy(0, 0, bTimes.getSizeInBytes()));
}

stats::Trace Tracer::GetStats() const
{
    stats::Trace result;

    static auto const timestampToMs = [pd = ctx.pd](u64 stamp) -> f32 {
        static auto const timestampValidBits { pd.getQueueFamilyProperties()[0].timestampValidBits };
        static auto const timestampPeriod { pd.getProperties().limits.timestampPeriod };

        if (timestampValidBits < sizeof(u64) * 8)
            stamp = stamp & ~(static_cast<u64>(-1) << timestampValidBits);
        return static_cast<f32>(stamp) * timestampPeriod * 1e-6f;
    };
    data_ptrace::TraceTimes const& data { *static_cast<data_ptrace::TraceTimes*>(bStaging.getMapping()) };
    data_ptrace::TraceStats const& stats { *static_cast<data_ptrace::TraceStats*>(bStats.getMapping()) };

    for (int i = 0; i < 8; i++) {
        result.data[i].rayCount = data.times[i].rayCount;
        result.data[i].traceTimeMs = timestampToMs(data.times[i].timer);
    }

    result.testedNodes = stats.testedNodes;
    result.testedTriangles = stats.testedTriangles;
    result.testedBVolumes = stats.testedBVolumes;

    return result;
}

void Tracer::SetVisualizationMode(State::VisMode mode)
{
    if (metadata.visMode == mode)
        return;

    metadata.visMode = mode;
    metadata.reallocRayBuffers = true;
    metadata.reloadPipelines = true;
}

void Tracer::checkForShaderHotReload()
{
    bool updated = false;

    updated = pipelines[Pipeline::eTraceSecondary].Update(ctx.d, ctx.sCache);
    if (updated) {
        vk::PipelineExecutableInfoKHR peInfo {
            .pipeline = pipelines[Pipeline::eTraceSecondary].get(),
            .executableIndex = 0,
        };
        auto stats = lime::check(ctx.d.getPipelineExecutableStatisticsKHR(peInfo));
        for (auto const& s : stats) {
            switch (s.format) {
            case vk::PipelineExecutableStatisticFormatKHR::eBool32:
                berry::log::warn("{}: {}", s.name.data(), s.value.b32);
                break;
            case vk::PipelineExecutableStatisticFormatKHR::eInt64:
                berry::log::warn("{}: (i32) {}, (i64) {}", s.name.data(), static_cast<i32>(s.value.i64), s.value.i64);
                break;
            case vk::PipelineExecutableStatisticFormatKHR::eUint64:
                berry::log::warn("{}: (u32) {}, (u64) {}", s.name.data(), static_cast<u32>(s.value.u64), s.value.u64);
                break;
            case vk::PipelineExecutableStatisticFormatKHR::eFloat64:
                berry::log::warn("{}: {}", s.name.data(), s.value.f64);
                break;
            }
        }
    }

    for (auto& p : pipelines)
        updated = p.Update(ctx.d, ctx.sCache) || updated;

    if (updated) {
        if (dSet)
            ctx.d.resetDescriptorPool(dPool.get());
        vk::DescriptorSetAllocateInfo allocInfo;
        allocInfo.descriptorPool = dPool.get();
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &pipelines[Pipeline::eGenPrimary].layout.dSet[0].get();
        dSet = lime::check(ctx.d.allocateDescriptorSets(allocInfo)).back();
    }
}

void Tracer::reloadPipelines()
{
    metadata.workgroupSize = CreateSpecializationConstants(ctx.pd);

    metadata.primaryRaysTileSize = { 32, 32 };
    // metadata.primaryRaysTileSize = { config.primaryRaysTileSize, config.primaryRaysTileSize };

    static std::array constexpr smePrimaryRays {
        vk::SpecializationMapEntry { 0, 0, sizeof(uint32_t) },
        vk::SpecializationMapEntry { 1, 4, sizeof(uint32_t) },
    };
    vk::SpecializationInfo siPrimaryRays { 2, smePrimaryRays.data(), 8, &metadata.primaryRaysTileSize };

    static std::array constexpr smeTrace {
        vk::SpecializationMapEntry { 0, 0, sizeof(uint32_t) },
    };
    vk::SpecializationInfo siTracePrimary { 1, smeTrace.data(), 4, &config.rPrimary.warpsPerWorkgroup };
    vk::SpecializationInfo siTraceSecondary { 1, smeTrace.data(), 4, &config.rSecondary.warpsPerWorkgroup };

    for (auto& p : pipelines)
        p = { };

    pipelines[Pipeline::eGenPrimary] = { ctx.d, ctx.sCache, config.shader.genPrimary, siPrimaryRays };

    switch (metadata.visMode) {
    case State::VisMode::ePathTracing:
        pipelines[Pipeline::eTracePrimary] = { ctx.d, ctx.sCache, config.shader.traceRays, siTracePrimary };
        pipelines[Pipeline::eTraceSecondary] = { ctx.d, ctx.sCache, config.shader.traceRays, siTraceSecondary };
        pipelines[Pipeline::eShadeAndCast] = { ctx.d, ctx.sCache, config.shader.shadeAndCast };
        break;
    case State::VisMode::eBVVisualization:
        pipelines[Pipeline::eTracePrimary] = { ctx.d, ctx.sCache, config.shader.traceRays_bv, siTracePrimary };
        pipelines[Pipeline::eTraceSecondary] = { ctx.d, ctx.sCache, config.shader.traceRays_bv, siTraceSecondary };
        pipelines[Pipeline::eShadeAndCast] = { ctx.d, ctx.sCache, config.shader.shadeAndCast_bv };
        break;
    case State::VisMode::eBVIntersections:
        pipelines[Pipeline::eTracePrimary] = { ctx.d, ctx.sCache, config.shader.traceRays_int, siTracePrimary };
        pipelines[Pipeline::eTraceSecondary] = { ctx.d, ctx.sCache, config.shader.traceRays_int, siTraceSecondary };
        pipelines[Pipeline::eShadeAndCast] = { ctx.d, ctx.sCache, config.shader.shadeAndCast_int };
        break;
    }

    if (dSet)
        ctx.d.resetDescriptorPool(dPool.get());
    vk::DescriptorSetAllocateInfo allocInfo;
    allocInfo.descriptorPool = dPool.get();
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &pipelines[Pipeline::eGenPrimary].layout.dSet[0].get();
    dSet = lime::check(ctx.d.allocateDescriptorSets(allocInfo)).back();
}

void Tracer::dSetUpdate(vk::ImageView targetImageView, lime::Buffer::Detail const& camera) const
{
    vk::DescriptorImageInfo dImage[1];
    dImage[0].imageView = targetImageView;
    dImage[0].imageLayout = vk::ImageLayout::eGeneral;

    vk::DescriptorBufferInfo dBuffer[1];
    dBuffer[0].buffer = camera.resource;
    dBuffer[0].offset = camera.offset;
    dBuffer[0].range = camera.size;

    vk::WriteDescriptorSet dWrite[2];
    dWrite[0].dstSet = dSet;
    dWrite[0].dstBinding = 0;
    dWrite[0].descriptorCount = 1;
    dWrite[0].descriptorType = vk::DescriptorType::eStorageImage;
    dWrite[0].pImageInfo = dImage;

    dWrite[1].dstSet = dSet;
    dWrite[1].dstBinding = 1;
    dWrite[1].descriptorCount = 1;
    dWrite[1].descriptorType = vk::DescriptorType::eUniformBufferDynamic;
    dWrite[1].pBufferInfo = dBuffer;

    ctx.d.updateDescriptorSets(2, dWrite, 0, nullptr);
}

void Tracer::allocRayBuffers(uint32_t rayCount)
{
    if (rayCount == 0)
        return;

    lime::AllocRequirements aReq {
        .memoryUsage = lime::DeviceMemoryUsage::eDeviceOptimal,
    };
    vk::BufferCreateInfo cInfo {
        .size = 0,
        .usage = bfub::eStorageBuffer | bfub::eShaderDeviceAddress,
    };

    metadata.rayCount = rayCount;

    cInfo.size = metadata.rayCount * sizeof(data_ptrace::Ray);
    bRay[0] = ctx.memory.alloc(aReq, cInfo, "tracer_ray_buffer_0");
    bRay[1] = ctx.memory.alloc(aReq, cInfo, "tracer_ray_buffer_1");

    cInfo.size = metadata.rayCount * sizeof(data_ptrace::RayPayload);
    bRayPayload[0] = ctx.memory.alloc(aReq, cInfo, "tracer_ray_payload_0");
    bRayPayload[1] = ctx.memory.alloc(aReq, cInfo, "tracer_ray_payload_1");

    switch (metadata.visMode) {
    case State::VisMode::ePathTracing:
        cInfo.size = metadata.rayCount * sizeof(data_ptrace::RayTraceResult);
        break;
    case State::VisMode::eBVVisualization:
        cInfo.size = metadata.rayCount * sizeof(data_ptrace::RayTraceResult_BV);
        break;
    case State::VisMode::eBVIntersections:
        cInfo.size = metadata.rayCount * sizeof(data_ptrace::RayTraceResult_Intersections);
        break;
    }

    bTraceResult = ctx.memory.alloc(aReq, cInfo, "tracer_result");
}

void Tracer::allocStatic()
{
    lime::AllocRequirements aReq {
        .memoryUsage = lime::DeviceMemoryUsage::eDeviceOptimal,
    };
    vk::BufferCreateInfo cInfo {
        .size = 0,
        .usage = bfub::eStorageBuffer | bfub::eShaderDeviceAddress | bfub::eTransferDst | bfub::eTransferSrc,
    };

    cInfo.size = sizeof(data_ptrace::RayBufferMetadata);
    bRayMetadata[0] = ctx.memory.alloc(aReq, cInfo, "tracer_ray_metadata_0");
    bRayMetadata[1] = ctx.memory.alloc(aReq, cInfo, "tracer_ray_metadata_1");

    cInfo.size = sizeof(data_ptrace::TraceTimes);
    bTimes = ctx.memory.alloc(aReq, cInfo, "tracer_times");

    aReq.memoryUsage = lime::DeviceMemoryUsage::eDeviceToHost;
    cInfo.usage = bfub::eTransferDst;
    bStaging = ctx.memory.alloc(aReq, cInfo, "tracer_staging");

    aReq.memoryUsage = lime::DeviceMemoryUsage::eHostToDeviceOptimal;
    cInfo.usage |= bfub::eShaderDeviceAddress;
    cInfo.size = sizeof(data_ptrace::TraceStats);
    bStats = ctx.memory.alloc(aReq, cInfo, "tracer_stats");
}

void Tracer::freeRayBuffers()
{
    for (auto& buf : bRay) {
        buf.reset();
    }
    for (auto& buf : bRayPayload) {
        buf.reset();
    }
    bTraceResult.reset();
}

void Tracer::freeAll()
{
    freeRayBuffers();

    for (auto& buf : bRayMetadata)
        buf.reset();
    // bStats.reset();
    bTimes.reset();
    bStaging.reset();
}
}
