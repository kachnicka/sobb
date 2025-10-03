#include "BvhBuilder.h"

#include "../../data/Scene.h"
#include <vLime/CommandPool.h>
#include <vLime/RenderGraph.h>

namespace backend::vulkan::bvh {

Bvh Builder::GetBvhForTraversal() const
{
    if (buildState != BuildState::eDone)
        return {};

    if (buildConfig.tracer.bv == config::BV::eNone)
        return {};

    return rearrangement.GetBVH();
}

void Builder::Configure(config::BVHPipeline config)
{
    buildConfig = std::move(config);

    [[maybe_unused]] auto t0 = rearrangement.NeedsRecompute(buildConfig.rearrangement);
    [[maybe_unused]] auto t1 = transformation.NeedsRecompute(buildConfig.transformation);
    [[maybe_unused]] auto t2 = collapsing.NeedsRecompute(buildConfig.collapsing);
    [[maybe_unused]] auto t3 = plocpp.NeedsRecompute(buildConfig.plocpp);

    if (t0 || t1 || t2 || t3) {
        buildState = BuildState::ePLOC;

        rearrangement.freeAll();
        transformation.freeAll();
        collapsing.freeAll();
        plocpp.freeAll();
    }
}

void Builder::CheckForShaderHotReload()
{
    if (rearrangement.CheckForShaderHotReload())
        buildState = BuildState::eRearrangement;
    if (transformation.CheckForShaderHotReload())
        buildState = BuildState::eTransformation;
    if (collapsing.CheckForShaderHotReload())
        buildState = BuildState::eCollapsing;
    if (plocpp.CheckForShaderHotReload())
        buildState = BuildState::ePLOC;
}

void Builder::scheduleBuildSteps()
{
    buildSteps.clear();
    switch (buildState) {
    case BuildState::ePLOC:
        if (buildConfig.plocpp.bv != config::BV::eNone)
            buildSteps.emplace_back(BuildState::ePLOC);
        [[fallthrough]];
    case BuildState::eCollapsing:
        if (buildConfig.collapsing.bv != config::BV::eNone && buildConfig.collapsing.maxLeafSize > 1)
            buildSteps.emplace_back(BuildState::eCollapsing);
        [[fallthrough]];
    case BuildState::eTransformation:
        if (buildConfig.transformation.bv != config::BV::eNone)
            buildSteps.emplace_back(BuildState::eTransformation);
        [[fallthrough]];
    case BuildState::eRearrangement:
        if (buildConfig.rearrangement.bv != config::BV::eNone)
            buildSteps.emplace_back(BuildState::eRearrangement);
        [[fallthrough]];
    default:
        break;
    }
    buildState = BuildState::eDone;
}

Bvh Builder::getIntermediateBvh(BuildState state) const
{
    switch (state) {
    case BuildState::eRearrangement:
        if (buildConfig.transformation.bv != config::BV::eNone)
            return transformation.GetBVH();
        [[fallthrough]];
    case BuildState::eTransformation:
        if (buildConfig.collapsing.bv != config::BV::eNone && buildConfig.collapsing.maxLeafSize > 1)
            return collapsing.GetBVH();
        [[fallthrough]];
    case BuildState::eCollapsing:
        return plocpp.GetBVH();
    default:
        return {};
    }
}

void Builder::BVHBuildPiecewise(lime::rg::Graph& rg, data::Scene const& scene)
{
    scheduleBuildSteps();
    if (buildSteps.empty())
        return;

    intermediateBvh = {};
    berry::log::debug("Scheduling BVH construction: {}", buildConfig.name);
    statsBuild.clear();
    stats.SetSceneAabbSurfaceArea(scene.aabb.Area());
    lime::rg::id::CommandsSync asTask;

    for (auto const& step : buildSteps) {
        switch (step) {
        case BuildState::ePLOC:
            asTask = rg.AddTask<lime::rg::CommandsSync>();
            rg.GetTask(asTask).RegisterExecutionCallback([this, &scene](vk::CommandBuffer commandBuffer) {
                berry::log::debug("BVH build stage: PLOCpp");
                plocpp.Compute(commandBuffer, scene);
            });
            asTask = rg.AddTask<lime::rg::CommandsSync>();
            rg.GetTask(asTask).RegisterExecutionCallback([this](vk::CommandBuffer commandBuffer) {
                plocpp.ReadRuntimeData();
                intermediateBvh = plocpp.GetBVH();

                plocpp.freeIntermediate();
                ctx.memory.cleanUp();

                berry::log::debug("BVH build stage: PLOCpp stats");
                buildConfig.stats.bv = buildConfig.plocpp.bv;
                stats.Compute(commandBuffer, buildConfig.stats, intermediateBvh);
            });
            asTask = rg.AddTask<lime::rg::CommandsSync>();
            rg.GetTask(asTask).RegisterExecutionCallback([this](vk::CommandBuffer commandBuffer) {
                static_cast<void>(commandBuffer);
                statsBuild.plocpp = plocpp.GatherStats(*stats.data);
            });
            break;
        case BuildState::eCollapsing:
            asTask = rg.AddTask<lime::rg::CommandsSync>();
            rg.GetTask(asTask).RegisterExecutionCallback([this, &scene](vk::CommandBuffer commandBuffer) {
                berry::log::debug("BVH build stage: Collapsing");
                if (!intermediateBvh.isValid())
                    intermediateBvh = getIntermediateBvh(BuildState::eCollapsing);
                auto const geometryDescriptorAddress { scene.data->sceneDescriptionBuffer.getDeviceAddress(ctx.d) };
                collapsing.Compute(commandBuffer, intermediateBvh, geometryDescriptorAddress);
            });
            asTask = rg.AddTask<lime::rg::CommandsSync>();
            rg.GetTask(asTask).RegisterExecutionCallback([this](vk::CommandBuffer commandBuffer) {
                collapsing.ReadRuntimeData();
                intermediateBvh = collapsing.GetBVH();

                collapsing.freeIntermediate();
                ctx.memory.cleanUp();

                berry::log::debug("BVH build stage: Collapsing stats");
                buildConfig.stats.bv = buildConfig.collapsing.bv;
                stats.Compute(commandBuffer, buildConfig.stats, intermediateBvh);
            });
            asTask = rg.AddTask<lime::rg::CommandsSync>();
            rg.GetTask(asTask).RegisterExecutionCallback([this](vk::CommandBuffer commandBuffer) {
                static_cast<void>(commandBuffer);
                statsBuild.collapsing = collapsing.GatherStats(*stats.data);
            });
            break;
        case BuildState::eTransformation:
            asTask = rg.AddTask<lime::rg::CommandsSync>();
            rg.GetTask(asTask).RegisterExecutionCallback([this, &scene](vk::CommandBuffer commandBuffer) {
                berry::log::debug("BVH build stage: Transformation");
                if (!intermediateBvh.isValid())
                    intermediateBvh = getIntermediateBvh(BuildState::eTransformation);
                auto const geometryDescriptorAddress { scene.data->sceneDescriptionBuffer.getDeviceAddress(ctx.d) };
                transformation.Compute(commandBuffer, intermediateBvh, geometryDescriptorAddress);
            });
            asTask = rg.AddTask<lime::rg::CommandsSync>();
            rg.GetTask(asTask).RegisterExecutionCallback([this](vk::CommandBuffer commandBuffer) {
                transformation.ReadRuntimeData();
                intermediateBvh = transformation.GetBVH();

                collapsing.freeIntermediate();
                transformation.freeIntermediate();
                ctx.memory.cleanUp();

                berry::log::debug("BVH build stage: Transformation stats");
                buildConfig.stats.bv = buildConfig.transformation.bv;
                stats.Compute(commandBuffer, buildConfig.stats, intermediateBvh);
            });
            asTask = rg.AddTask<lime::rg::CommandsSync>();
            rg.GetTask(asTask).RegisterExecutionCallback([this](vk::CommandBuffer commandBuffer) {
                static_cast<void>(commandBuffer);
                statsBuild.transformation = transformation.GatherStats(*stats.data);
            });
            break;
        case BuildState::eRearrangement:
            asTask = rg.AddTask<lime::rg::CommandsSync>();
            rg.GetTask(asTask).RegisterExecutionCallback([this](vk::CommandBuffer commandBuffer) {
                berry::log::debug("BVH build stage: Rearrangement");
                if (!intermediateBvh.isValid())
                    intermediateBvh = getIntermediateBvh(BuildState::eRearrangement);
                rearrangement.Compute(commandBuffer, intermediateBvh);
            });
            asTask = rg.AddTask<lime::rg::CommandsSync>();
            rg.GetTask(asTask).RegisterExecutionCallback([this](vk::CommandBuffer commandBuffer) {
                rearrangement.ReadRuntimeData();
                intermediateBvh = rearrangement.GetBVH();

                rearrangement.freeIntermediate();
                transformation.freeIntermediate();
                ctx.memory.cleanUp();

                berry::log::debug("BVH build stage: Rearrangement stats");
                buildConfig.stats.bv = buildConfig.rearrangement.bv;
                stats.Compute(commandBuffer, buildConfig.stats, intermediateBvh);
            });
            asTask = rg.AddTask<lime::rg::CommandsSync>();
            rg.GetTask(asTask).RegisterExecutionCallback([this](vk::CommandBuffer commandBuffer) {
                static_cast<void>(commandBuffer);
                statsBuild.rearrangement = rearrangement.GatherStats(*stats.data);
            });
            break;
        case BuildState::eDone:
            break;
        }
    }
}

}
