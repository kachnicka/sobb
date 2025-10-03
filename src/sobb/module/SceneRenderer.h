#pragma once

#include "../ApplicationState.h"
#include "../backend/vulkan/Vulkan.h"
#include "../core/GUI.h"
#include "../scene/Camera.h"

namespace module {

class SceneGraph;

class SceneRenderer {
private:
    void guiRayTracerShared();
    void guiPathTracerCompute();

public:
    backend::vulkan::Vulkan& backend;
    gui::RendererIOWidget window;

public:
    explicit SceneRenderer(backend::vulkan::Vulkan& backend)
        : backend(backend)
    {
    }

    void ProcessGUI(State&);

    void RenderFrame()
    {
        backend.SetCamera(window.camera.GetMatrices());
    }

    Camera& GetCamera()
    {
        return window.camera;
    }
};

}
