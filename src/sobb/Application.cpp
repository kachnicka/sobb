#include "Application.h"

#include "core/GUI.h"
#include "scene/SceneIO.h"
#include "util/pexec.h"

#include <berries/lib_helper/spdlog.h>
#include <filesystem>

#include "image/Writer.h"
#include "scene/Serialization.h"

namespace sobb_glfw {

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    static_cast<void>(scancode);
    static_cast<void>(mods);

    auto* app { static_cast<Application*>(glfwGetWindowUserPointer(window)) };

    switch (key) {
    case GLFW_KEY_ESCAPE:
        if (action == GLFW_PRESS)
            app->Exit();
        break;
    default:;
    }
}

static void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    auto* app { static_cast<Application*>(glfwGetWindowUserPointer(window)) };
    berry::log::debug("Resize: Main window, {}x{} to {}x{}", app->window.width, app->window.height, width, height);
    app->window.width = width;
    app->window.height = height;
    app->state.windowSize.x = static_cast<u32>(width);
    app->state.windowSize.y = static_cast<u32>(height);
    app->backend.ResizeSwapChain(static_cast<u32>(width), static_cast<u32>(height));
}

}

Application::RuntimeDirectory::RuntimeDirectory(int argc, char* argv[])
    : bin(pexec::get_path_to_executable().parent_path())
    , cwd(std::filesystem::current_path())
{
    static_cast<void>(argc);
    static_cast<void>(argv);

    // locate resource dir
    auto path_to_explore { bin };
    auto const root { bin.root_directory() };

    if (res = bin / "data"; std::filesystem::exists(res))
        goto log_dirs;
    if (res = cwd / "data"; std::filesystem::exists(res))
        goto log_dirs;
    while (path_to_explore != root) {
        path_to_explore = path_to_explore.parent_path();
        if (res = path_to_explore / "data"; std::filesystem::exists(res))
            goto log_dirs;
    }
log_dirs:
    berry::log::info("bin: {}", bin.generic_string());
    berry::log::info("cwd: {}", cwd.generic_string());
    berry::log::info("res: {}", res.generic_string());

    if (path_to_explore == root) {
        berry::log::error("Can't locate '/data' folder.");
        exit(EXIT_FAILURE);
    }
}

static constexpr int WIDTH { 1920 };
static constexpr int HEIGHT { 1080 };
constexpr char const* defaultScenes { "scene.toml" };
constexpr char const* defaultConfig { "benchmark.toml" };

Application::Application(int argc, char* argv[])
    : window(WIDTH, HEIGHT, "SOBB", sobb_glfw::keyCallback)
    , directory(argc, argv)
    , configFiles(directory.res, directory.res / defaultScenes, directory.res / defaultConfig)
    , shaderManager(directory.res)
    , backend(window, shaderManager)
    , sceneRenderer(backend)
    , cameraManager(sceneRenderer.GetCamera())
    , benchmark(*this)
    , animation(*this)
{
    glfwSetFramebufferSizeCallback(window.window, sobb_glfw::framebufferSizeCallback);
    glfwSetWindowUserPointer(window.window, this);

    state.sceneToLoad = configFiles.GetScene();

    gui::create(window);
    backend.InitGUIRenderer((directory.res / "font/cascadia/CascadiaMono.ttf").string().c_str());
}

int Application::Run()
{
    double prevTime { 0. };
    while (!window.ShouldClose()) {

        state.time = glfwGetTime();
        state.deltaTime = state.time - prevTime;
        prevTime = state.time;

        if (!state.sceneToLoad.name.empty()) {
            unloadScene();
            loadScene(state.sceneToLoad);
            state.sceneToLoad = {};
        }

        benchmark.Run();
        animation.Run();

        glfwPollEvents();
        asyncProcessing.Check();
        gui();

        sceneRenderer.RenderFrame();

        backend.SubmitFrame();
        state.frameId++;
    }

    backend.WaitIdle();
    return EXIT_SUCCESS;
}

void Application::Exit()
{
    if (State::FAST_EXIT) {
        asyncProcessing.wait_for_all(std::chrono::milliseconds(100));
        exit(EXIT_SUCCESS);
    }
    glfwSetWindowShouldClose(window.window, GLFW_TRUE);
}

void Application::AsyncProcessing::Check()
{
    constexpr auto isReady {
        [](auto const& f) {
            return f.wait_for(std::chrono::nanoseconds(1)) == std::future_status::ready;
        }
    };

    std::vector<Future<std::optional<HostScene>>> notReady;
    for (auto& s : scenes) {
        if (isReady(s)) {
            app.scenes.emplace_back(std::make_unique<HostScene>(std::move(s.get().value())));
            berry::log::timer("Scene load finished", glfwGetTime());
            app.state.sceneLoading = false;
            app.cameraManager.camera.Fit(app.scenes.back()->aabb);
        } else
            notReady.emplace_back(std::move(s));
    }
    scenes = std::move(notReady);

    if (fHotReload.valid() && isReady(fHotReload))
        fHotReload.get();
    if (!fHotReload.valid() && app.state.shaderHotReload && app.backend.state.oncePer250ms)
        fHotReload = app.mainExecutor.async([this]() {
            app.shaderManager.CheckForHotReload();
        });
}

bool Application::AsyncProcessing::wait_for_all(std::chrono::milliseconds timeout)
{
    auto future = std::async(std::launch::async, [&]() {
        app.mainExecutor.wait_for_all();
    });

    if (future.wait_for(timeout) == std::future_status::timeout) {
        return false;
    }
    return true;
}

void Application::loadScene(ConfigFiles::Scene const& scene)
{
    state.sceneLoading = true;
    state.statusBarFade = 1.0f;
    if (!scene.bin.empty())
        loadSceneBinary(scene.bin);
    else
        loadSceneAssimp(scene.path);
    cameraManager.LoadCameraFile(scene.camera);
    benchmark.SetScene(scene.name);
}

void Application::loadSceneBinary(std::string_view path)
{
    if (!std::filesystem::exists(path)) {
        berry::log::error("File does not exist: {}", path);
        return;
    }
    berry::log::timer("Scene load started (binary)", glfwGetTime());
    asyncProcessing.scenes.push_back(mainExecutor.async([this, path = std::filesystem::path(path)]() {
        HostScene result { scene::deserialize(path) };
        berry::log::timer("  deserialized", glfwGetTime());

        result.RecomputeWorldMatrices();
        backend.UploadScene(result);
        berry::log::timer("  uploaded to GPU", glfwGetTime());
        backend.ResetAccumulation();

        result.path = path;
        return result;
    }));
}

void Application::loadSceneAssimp(std::string_view path)
{
    if (!std::filesystem::exists(path)) {
        berry::log::error("File does not exist: {}", path);
        return;
    }
    berry::log::timer("Scene load started", glfwGetTime());
    asyncProcessing.scenes.push_back(mainExecutor.async([this, path = std::filesystem::path(path)]() {
        HostScene result;
        SceneIO io;

        berry::log::timer("  importing..", glfwGetTime());
        io.ImportScene(path.generic_string(), &state.progress);
        berry::log::timer("  imported", glfwGetTime());
        result = io.CreateScene();
        berry::log::timer("  created", glfwGetTime());

        result.RecomputeWorldMatrices();
        backend.UploadScene(result);
        berry::log::timer("  uploaded to GPU", glfwGetTime());
        backend.ResetAccumulation();

        result.path = path;
        return result;
    }));
}

void Application::unloadScene()
{
    if (scenes.empty())
        return;

    backend.UnloadScene();
    backend.ResizeRenderer(sceneRenderer.window.camera.screenResolution.x, sceneRenderer.window.camera.screenResolution.y);

    scenes.clear();

    cameraManager.ClearCameras();
}

void Application::gui()
{
    gui::new_frame();
    ProcessGUI();
    cameraManager.ProcessGUI(state);
    sceneRenderer.ProcessGUI(state);
    benchmark.ProcessGUI(state);
    animation.ProcessGUI(state);
    gui::end_frame();
}

void Application::saveRenderWindowAsImage(std::filesystem::path const& path)
{
    auto const x { sceneRenderer.GetCamera().screenResolution.x };
    auto const y { sceneRenderer.GetCamera().screenResolution.y };
    std::vector<glm::vec4> imgResult;
    imgResult.resize(x * y);
    backend.GetImageData(imgResult);

    std::vector<uint8_t> ldrImg;
    ldrImg.reserve(x * y * 4);

    // NOTE: hardcoded reinhard operator for export, image does not match the rendered visuals
    for (auto& c : imgResult) {
        // glm::max(c, glm::vec4(0.f, 0.f, 0.f, 0.f));
        // glm::min(c, glm::vec4(1.f, 1.f, 1.f, 1.f));
        c.x /= c.x + 1.f;
        c.y /= c.y + 1.f;
        c.z /= c.z + 1.f;
        c *= 255.f;
        // ldrImg.emplace_back(std::clamp(static_cast<uint8_t>(c.x), static_cast<uint8_t>(0), static_cast<uint8_t>(255)));
        // ldrImg.emplace_back(std::clamp(static_cast<uint8_t>(c.y), static_cast<uint8_t>(0), static_cast<uint8_t>(255)));
        // ldrImg.emplace_back(std::clamp(static_cast<uint8_t>(c.z), static_cast<uint8_t>(0), static_cast<uint8_t>(255)));
        ldrImg.emplace_back(static_cast<uint8_t>(c.x));
        ldrImg.emplace_back(static_cast<uint8_t>(c.y));
        ldrImg.emplace_back(static_cast<uint8_t>(c.z));
        ldrImg.emplace_back(static_cast<uint8_t>(255));
    }
    std::string pathString { path.generic_string() };
    if (path.extension() != ".png")
        pathString.append(".png");
    berry::log::info("Saving image: {}", pathString);
    image::write(pathString.c_str(), x, y, 4, ldrImg.data(), x * 4);
}
