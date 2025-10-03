#include "Renderer_ptCompute.h"

#include "../VulkanState.h"
#include "../data/Scene.h"

#include <vLime/RenderGraph.h>

namespace backend::vulkan {

void PathTracerCompute::Update(State const& state)
{
    tracer.dirLight = state.dirLight;
    traceRuntimeData.ptDepth = state.ptDepth;
    traceRuntimeData.samples.computed = state.samplesComputed;
    traceRuntimeData.samples.toCompute = state.samplesToComputeThisFrame;

    tracer.SetVisualizationMode(state.selectedVisMode);

    if (postponeHotReloadOneFrame)
        postponeHotReloadOneFrame = false;
    else
        builder.CheckForShaderHotReload();
}

lime::rg::id::Resource PathTracerCompute::ScheduleRgTasks(lime::rg::Graph& rg, data::Scene const& scene)
{
    lime::rg::id::Commands ptTask;

    if (scene.changed.everything) {
        builder.buildState = bvh::Builder::BuildState::ePLOC;
    }
    builder.BVHBuildPiecewise(rg, scene);

    ptImg = rg.AddResource();
    rg.GetResource(ptImg).BindToPhysicalResource(img);
    rg.GetResource(ptImg).finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

    if (traceRuntimeData.samples.toCompute == 0)
        return ptImg;

    // transition the image to general layout
    auto const transitionToGeneralTask { rg.AddTask<lime::rg::Commands>() };
    rg.GetTask(transitionToGeneralTask).RegisterExecutionCallback([this, &rg](vk::CommandBuffer commandBuffer) {
        auto const currentLayout { img.layout };
        auto const finalLayout { vk::ImageLayout::eGeneral };
        auto const accessFlags = lime::img_util::DefaultAccessFlags(currentLayout, finalLayout);
        vk::ImageMemoryBarrier memoryBarrier {
            .srcAccessMask = accessFlags.first,
            .dstAccessMask = accessFlags.second,
            .oldLayout = currentLayout,
            .newLayout = finalLayout,
            .image = rg.GetResource(ptImg).physicalResourceDetail.back().image,
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
        commandBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::PipelineStageFlagBits::eComputeShader,
            {}, nullptr, nullptr, memoryBarrier);
        img.layout = finalLayout;
    });

    ptTask = rg.AddTask<lime::rg::Commands>();
    rg.SetTaskResourceOperation(ptTask, ptImg, lime::rg::Resource::OperationFlagBits::eStored);

    rg.GetTask(ptTask).RegisterExecutionCallback([this, &rg, &scene](vk::CommandBuffer commandBuffer) {
        auto const bvh { builder.GetBvhForTraversal() };
        if (!bvh.isValid())
            return;

        traceRuntimeData.geometryDescriptorAddress = scene.data->sceneDescriptionBuffer.getDeviceAddress(ctx.d);
        traceRuntimeData.camera = scene.data->cameraBuffer;
        traceRuntimeData.x = rg.GetResource(ptImg).extent.width;
        traceRuntimeData.y = rg.GetResource(ptImg).extent.height;

        statsTrace = tracer.GetStats();
        tracer.Trace(commandBuffer, builder.GetConfig().tracer, traceRuntimeData, bvh);
    });

    // transition to final layout
    auto const transitionToFinalTask { rg.AddTask<lime::rg::Commands>() };
    rg.GetTask(transitionToFinalTask).RegisterExecutionCallback([this, &rg](vk::CommandBuffer commandBuffer) {
        auto const currentLayout { vk::ImageLayout::eGeneral };
        auto const finalLayout { rg.GetResource(ptImg).finalLayout };
        auto const accessFlags = lime::img_util::DefaultAccessFlags(currentLayout, finalLayout);
        vk::ImageMemoryBarrier memoryBarrier {
            .srcAccessMask = accessFlags.first,
            .dstAccessMask = accessFlags.second,
            .oldLayout = currentLayout,
            .newLayout = finalLayout,
            .image = rg.GetResource(ptImg).physicalResourceDetail.back().image,
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
        commandBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eFragmentShader,
            {}, nullptr, nullptr, memoryBarrier);
        img.layout = finalLayout;
    });

    return ptImg;
}

void PathTracerCompute::Resize(u32 x, u32 y)
{
    img.reset();
    vk::ImageCreateInfo cInfo {
        .flags = {},
        .imageType = vk::ImageType::e2D,
        .format = vk::Format::eR32G32B32A32Sfloat,
        .extent = { x, y, 1 },
        .mipLevels = 1,
        .arrayLayers = 1,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage,
        .initialLayout = vk::ImageLayout::eUndefined,
    };

    img = ctx.memory.alloc({ .memoryUsage = lime::DeviceMemoryUsage::eDeviceOptimal }, cInfo, "image_ptCompute");
    img.CreateImageView();

    traceRuntimeData.targetImageView = img.getView();
}

void PathTracerCompute::SetPipelineConfiguration(config::BVHPipeline config)
{
    builder.Configure(std::move(config));
    postponeHotReloadOneFrame = true;
}

}
