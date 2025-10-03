#pragma once

#include <memory>
#include <vector>

#include "ApplicationState.h"

#include "core/ConfigFiles.h"
#include "core/Taskflow.h"

#include "scene/Scene.h"

#include "module/Animation.h"
#include "module/Benchmark.h"
#include "module/CameraManager.h"
#include "module/SceneRenderer.h"
#include "module/ShaderManager.h"

#include <berries/lib_helper/glfw.h>

class Application {
public:
    berry::Window window;

    struct RuntimeDirectory {
        std::filesystem::path bin;
        std::filesystem::path cwd;
        std::filesystem::path res;

        RuntimeDirectory(int argc, char* argv[]);
    } directory;
    ConfigFiles configFiles;

    module::ShaderManager shaderManager;
    backend::vulkan::Vulkan backend;

    module::SceneRenderer sceneRenderer;
    module::CameraManager cameraManager;
    module::Benchmark benchmark;
    module::Animation animation;

    Executor mainExecutor;
    Taskflow perFrameTasks;

    struct {
        Task renderFrame;
    } task;

    struct AsyncProcessing {
    private:
        Application& app;
        Future<void> fHotReload;

    public:
        std::vector<Future<std::optional<HostScene>>> scenes;

        explicit AsyncProcessing(Application& app)
            : app(app)
        {
        }

        void Check();
        [[maybe_unused]] bool wait_for_all(std::chrono::milliseconds timeout = std::chrono::milliseconds(1));
    } asyncProcessing { *this };

    std::vector<std::unique_ptr<HostScene>> scenes;
    State state;

    void saveRenderWindowAsImage(std::filesystem::path const& path);

private:
    void gui();

public:
    explicit Application(int argc, char* argv[]);
    int Run();
    void Exit();

    void ProcessGUI();

    void toggleAppMode();

    void loadScene(ConfigFiles::Scene const& scene);
    void loadSceneAssimp(std::string_view path);
    void loadSceneBinary(std::string_view path);
    void unloadScene();
};
