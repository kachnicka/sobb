#pragma once

#include <vLime/Queues.h>
#include <vLime/RenderGraph.h>

#include "VCtx.h"
#include "data/ImGuiScene.h"
#include "workload/ImGui.h"

namespace backend::input {
struct Camera;
}
namespace backend::vulkan {
struct Vulkan;
}

namespace backend::vulkan::task_old_style {

struct ImGui {
    lime::rg::id::Rasterization task;
    lime::rg::id::Resource imgRendered;
    lime::rg::id::Resource imgTarget;

    Renderer_ImGui renderer;
    data::Scene_imGUI scene;

    explicit ImGui(VCtx ctx)
        : renderer(ctx.d, ctx.sCache)
        , scene(ctx)
    {
    }

    void scheduleRgTask(lime::rg::Graph& rg);
};

}
