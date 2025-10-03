#pragma once

#include "../Config.h"
#include "VulkanState.h"

#include <berries/lib_helper/glfw.h>
#include <berries/lib_helper/spdlog.h>
#include <berries/util/types.h>
#include <glm/ext/vector_float4.hpp>

struct HostScene;

namespace module {
class ShaderManager;
}

namespace backend::input {
struct Camera;
}

namespace backend::stats {
struct BVHPipeline;
struct Trace;
}

namespace backend::vulkan {

struct Vulkan {
    Vulkan(berry::Window const& window, module::ShaderManager& shaman);
    ~Vulkan();

    void SubmitFrame();
    void WaitIdle() const;
    void ProcessGUI();

    void InitGUIRenderer(char const* pathFont);

    config::BVHPipeline GetConfig();
    stats::BVHPipeline GetStatsBuild() const;
    stats::Trace GetStatsTrace() const;

    void GetImageData(std::vector<glm::vec4>& result);

    void UploadScene(HostScene const& scene) const;
    void UnloadScene();
    void UpdateTransformationMatrices(HostScene const& scene);
    void SetPipelineConfiguration(config::BVHPipeline config);
    void SetCamera(input::Camera const& c);

    void SetVSync(bool enable, u32 x, u32 y);
    void SelectNode(u32 nodeIndex);
    [[nodiscard]] bool HaveScene() const;

    void ResizeSwapChain(u32 x, u32 y) const;
    void ResizeRenderer(u32 x, u32 y) const;

    [[nodiscard]] bool IsRendering() const;
    void ResetAccumulation();
    void CalculateSampleCountsForFrame() const;
    void OnRenderModeChange() const;

    State state;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

}
