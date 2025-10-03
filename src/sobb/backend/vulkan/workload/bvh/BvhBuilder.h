#pragma once

#include "Collapsing.h"
#include "PLOCpp.h"
#include "Rearrangement.h"
#include "Stats.h"
#include "Transformation.h"

namespace lime::rg {
class Graph;
}
namespace backend::vulkan {
struct Vulkan;
}

namespace backend::vulkan::bvh {

class Builder {
    VCtx ctx;
    lime::Queue queue;

    PLOCpp plocpp;
    Collapsing collapsing;
    Transformation transformation;
    Rearrangement rearrangement;
    Stats stats;

    Bvh intermediateBvh;

public:
    enum class BuildState {
        eDone,
        ePLOC,
        eCollapsing,
        eTransformation,
        eRearrangement,
    } buildState { BuildState::ePLOC };
    std::vector<BuildState> buildSteps;

    config::BVHPipeline buildConfig;
    stats::BVHPipeline statsBuild;

public:
    explicit Builder(VCtx ctx, lime::Queue queue)
        : ctx(ctx)
        , queue(queue)
        , plocpp(ctx)
        , collapsing(ctx)
        , transformation(ctx)
        , rearrangement(ctx)
        , stats(ctx)
    {
        buildSteps.reserve(4);
    }

    stats::BVHPipeline const& GetStatsBuild() const
    {
        return statsBuild;
    }

    config::BVHPipeline const& GetConfig() const
    {
        return buildConfig;
    }

    Bvh GetBvhForTraversal() const;

    void Configure(config::BVHPipeline config);
    void CheckForShaderHotReload();
    void BVHBuildPiecewise(lime::rg::Graph& rg, data::Scene const& scene);

private:
    void scheduleBuildSteps();
    Bvh getIntermediateBvh(BuildState state) const;
};

}
