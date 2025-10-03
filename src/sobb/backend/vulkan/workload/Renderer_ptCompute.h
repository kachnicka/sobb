#pragma once

#include <vLime/RenderGraph.h>

#include "../VCtx.h"
#include "Renderer.h"
#include "bvh/BvhBuilder.h"
#include "bvh/Tracer.h"

namespace backend::vulkan {

struct PathTracerCompute final : public Renderer {
private:
    VCtx ctx;

    bvh::Builder builder;
    bvh::Tracer tracer;
    bvh::TraceRuntime traceRuntimeData;
    stats::Trace statsTrace;

    lime::Image img;
    lime::rg::id::Resource ptImg;

    bool postponeHotReloadOneFrame { false };
public:
    PathTracerCompute(VCtx ctx, lime::Queue queue)
        : ctx(ctx)
        , builder(ctx, queue)
        , tracer(ctx)
    {
    }

    void Update(State const& state) override;
    lime::rg::id::Resource ScheduleRgTasks(lime::rg::Graph& rg, data::Scene const& scene) override;
    void Resize(u32 x, u32 y) override;
    lime::Image const& GetBackbuffer() const override
    {
        return img;
    }

    stats::BVHPipeline const& GetStatsBuild() const
    {
        return builder.statsBuild;
    }

    stats::Trace const& GetStatsTrace() const
    {
        return statsTrace;
    }

    config::BVHPipeline GetConfig() const
    {
        return builder.buildConfig;
    }

    void SetPipelineConfiguration(config::BVHPipeline config);

};

}
