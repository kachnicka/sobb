#include "Vulkan.h"

#include "MyCapabilities.h"
#include "RadixSort.h"
#include "Task.h"
#include "VCtx.h"
#include "data/DeviceData.h"

#include "../../module/ShaderManager.h"

#include <memory>

#include <berries/lib_helper/glfw.h>
#include <berries/lib_helper/spdlog.h>
#include <glm/gtc/type_ptr.hpp>
#include <vLime/ExtensionsAndLayers.h>
#include <vLime/Memory.h>
#include <vLime/Reflection.h>
#include <vLime/RenderGraph.h>
#include <vLime/SwapChain.h>
#include <vLime/Transfer.h>
#include <vLime/vLime.h>

#include "data/Scene.h"
#include "workload/Renderer.h"
#include "workload/Renderer_ptCompute.h"

namespace backend::vulkan {

struct Vulkan::Impl {
    State& state;

    lime::Capabilities capabilities;
    lime::Instance instance;
    lime::Device device;

    vk::Instance i;
    vk::PhysicalDevice pd;
    vk::Device d;

    lime::MemoryManager memory;
    lime::Transfer transfer;
    lime::ShaderCache sCache;
    lime::SwapChain swapChain;

    lime::Frame frame;
    lime::rg::Graph rg;
    lime::rg::id::Resource rgSwapChain;
    bool transitionSwapChain { true };

    u32 rx { 64 };
    u32 ry { 64 };
    std::unique_ptr<Renderer> renderer { nullptr };

    std::unique_ptr<task_old_style::ImGui> imgui { nullptr };

    std::unique_ptr<data::Scene> sceneOnDevice;
    data::DeviceData deviceData;

    Impl(State& state, berry::Window const& window, module::ShaderManager& shaman)
        : state(state)
        , capabilities(setupVulkanBackend())
        , instance(capabilities)
        , device(createDevice(lime::ListAllPhysicalDevicesInGroups(instance.get())[0][0], capabilities))
        , i(instance.get())
        , pd(device.getPd())
        , d(device.get())
        , memory(createMemoryManager(d, pd, capabilities))
        , transfer(device.queues.transfer, memory)
        , sCache(d, [&shaman](auto name, auto& data, auto callback) { shaman.Load(name, data, std::move(callback)); }, [&shaman](auto name) { shaman.Unload(name); })
        , swapChain(i, d, pd, limeWindow(window))
        , frame(d, device.queues.graphics, swapChain.GetImageCount())
        , rg(d, memory, device.queues.graphics)
        , deviceData(ctx())
    {
        lime::PrintAllPhysicalDevices(i);
        FuchsiaRadixSort::RadixSortCreate(d, sCache.getPipelineCache());

        RecreateRenderer();
    }

    ~Impl()
    {
        FuchsiaRadixSort::RadixSortDestroy(d);
    }

    void SubmitFrame()
    {
        if (sCache.CheckForHotReload())
            ResetAccumulation();
        CalculateSampleCountsForFrame();

        rg.reset();

        rgSwapChain = rg.AddResource();
        rg.GetResource(rgSwapChain).BindToPhysicalResource(swapChain.GetAllImages());
        rg.GetResource(rgSwapChain).finalLayout = vk::ImageLayout::ePresentSrcKHR;

        if (transitionSwapChain) {
            swapChain.scheduleRgTaskLayoutTransition(rg, vk::ImageLayout::ePresentSrcKHR);
            transitionSwapChain = false;
        }

        auto const [frameId, commandBuffer] { frame.ResetAndBeginCommandsRecording() };
        auto const backBufferDetail { swapChain.GetNextImage(frame.imageAvailableSemaphore.get()) };
        bool const haveImageToRenderTo { backBufferDetail.image };
        u32 const imgId { swapChain.imageIndexToPresentNext };

        if (haveImageToRenderTo) {
            lime::rg::id::Resource renderedImg;
            if (sceneOnDevice && renderer) {
                sceneOnDevice->changed.set(frameId);
                renderer->Update(state);

                renderedImg = renderer->ScheduleRgTasks(rg, *sceneOnDevice);
                imgui->scene.textureRenderedScene = renderer->GetBackbuffer().getView();
                imgui->imgRendered = renderedImg;
            }

            imgui->imgTarget = rgSwapChain;
            imgui->scheduleRgTask(rg);
            if (!renderedImg.id.isValid())
                rg.GetResource(imgui->imgRendered).BindToPhysicalResource(deviceData.textures.GetDefaultTextureDetail());

            rg.Compile();
            rg.SetupExecution(commandBuffer, imgId);
        }

        frame.EndCommandsRecording();
        auto const signalSemaphores = frame.SubmitCommands(imgId, true, haveImageToRenderTo);

        swapChain.Present(device.queues.graphics.q, signalSemaphores);
        frame.Wait();

        if (sceneOnDevice)
            sceneOnDevice->changed.reset();
    }

    void InitGUIRenderer(char const* pathFont)
    {
        imgui = std::make_unique<task_old_style::ImGui>(ctx());
        imgui->scene.SetDefaultTexture(deviceData.textures.GetDefaultTextureImageView());
        imgui->scene.upload_font_texture(deviceData.textures, pathFont);
    }

    data::Texture::ID AddTexture(uint32_t x, uint32_t y, char const* data, Format format)
    {
        return deviceData.textures.add(x, y, data, toVkFormat(format));
    }

    data::Geometry::ID AddGeometry(input::Geometry const& g)
    {
        return deviceData.geometries.add(g);
    }

    void SetViewerMode()
    {
        if (imgui) {
            deviceData.textures.remove(imgui->scene.fontId);
            imgui.reset();
        }
    }

    void setVSync(bool enable, u32 x, u32 y)
    {
        swapChain.vSync = enable;
        WaitIdle();
        swapChain.Resize(x, y);
        transitionSwapChain = true;
    }

    void ResizeSwapChain(u32 x, u32 y)
    {
        lime::check(device.queues.graphics.q.waitIdle());
        swapChain.Resize(x, y);
        transitionSwapChain = true;
    }

    void ResizeRenderer(u32 x, u32 y)
    {
        rx = x;
        ry = y;
        if (renderer)
            renderer->Resize(x, y);
    }

    void RecreateRenderer()
    {
        renderer.reset();
        switch (state.selectedRenderer) {
        case State::Renderer::ePathTracingCompute:
            if (!capabilities.isAvailable<RayTracing_compute>())
                return;
            renderer = std::make_unique<PathTracerCompute>(ctx(), device.queues.graphics);
            static_cast<PathTracerCompute*>(renderer.get())->SetPipelineConfiguration(state.ptComputeConfig);
            break;
        }
        if (renderer)
            renderer->Resize(rx, ry);
    }

    void WaitIdle() const
    {
        device.WaitIdle();
    }

    void CalculateSampleCountsForFrame()
    {
        if (state.resetAccumulation) {
            state.samplesComputed = 0;
            u32 samplesPerFrame { 1 };
            state.samplesToComputeThisFrame = std::min(state.samplesPerPixel, samplesPerFrame);
            state.resetAccumulation = false;
        } else {
            state.samplesComputed += state.samplesToComputeThisFrame;
            state.samplesToComputeThisFrame = std::min(state.samplesPerPixel - state.samplesComputed, state.samplesToComputeThisFrame);
        }
    }

    void UploadScene(HostScene const& sceneOnHost)
    {
        auto scene = std::make_unique<data::Scene>(&deviceData, sceneOnHost);
        scene->changed.loadedOnFrame = frame.CurrentFrameId();
        sceneOnDevice = std::move(scene);
    }

    void UnloadScene()
    {
        sceneOnDevice.reset();
        deviceData.reset();

        RecreateRenderer();

        imgui->scene.textureRenderedScene = deviceData.textures.GetDefaultTextureImageView();
        memory.cleanUp();
    }

    inline config::BVHPipeline GetConfig()
    {
        if (state.selectedRenderer != State::Renderer::ePathTracingCompute)
            return {};
        return static_cast<PathTracerCompute*>(renderer.get())->GetConfig();
    }

    inline stats::BVHPipeline GetStatsBuild() const
    {
        if (state.selectedRenderer != State::Renderer::ePathTracingCompute)
            return {};
        return static_cast<PathTracerCompute*>(renderer.get())->GetStatsBuild();
    }

    inline stats::Trace GetStatsTrace() const
    {
        if (state.selectedRenderer != State::Renderer::ePathTracingCompute)
            return {};
        return static_cast<PathTracerCompute*>(renderer.get())->GetStatsTrace();
    }

    inline void SetPipelineConfiguration(config::BVHPipeline config)
    {
        state.ptComputeConfig = std::move(config);
        if (state.selectedRenderer == State::Renderer::ePathTracingCompute) {
            static_cast<PathTracerCompute*>(renderer.get())->SetPipelineConfiguration(state.ptComputeConfig);
            ResetAccumulation();
        }
    }

    inline void SetCamera(input::Camera const& c)
    {
        deviceData.SetCamera_TMP(c);
    }

    void ProcessGUI();

    [[nodiscard]] inline bool IsRendering() const
    {
        return state.samplesToComputeThisFrame > 0;
    }

    inline void ResetAccumulation()
    {
        state.resetAccumulation = true;
    }

    static lime::Capabilities setupVulkanBackend()
    {
        lime::LoadVulkan();
        lime::log::SetCallbacks(&berry::log::info, &berry::log::debug, &berry::log::error);

        lime::Capabilities capabilities;
        capabilities.add<BasicGraphics>();
        capabilities.add<RayTracing_KHR>();
        capabilities.add<RayTracing_compute>();
        capabilities.add<FuchsiaRadixSort>();
        capabilities.add<OnScreenPresentation>();
        capabilities.add<ExecutableProperties>();

        return capabilities;
    }

    static lime::Device createDevice(vk::PhysicalDevice pd, lime::Capabilities& capabilities)
    {
        FuchsiaRadixSort::RadixSortInit(pd);
        return { pd, capabilities };
    }

    static lime::MemoryManager createMemoryManager(vk::Device d, vk::PhysicalDevice pd, lime::Capabilities& capabilities)
    {
        lime::MemoryManager memory(d, pd);
        auto const dFeatures { lime::device::CheckAndSetDeviceFeatures(capabilities, pd, true) };
        if (dFeatures.vulkan12Features.bufferDeviceAddress)
            memory.features.bufferDeviceAddress = true;
        return memory;
    }

    [[nodiscard]] VCtx ctx()
    {
        return { d, pd, memory, transfer, sCache, capabilities };
    }

    static lime::Window limeWindow(berry::Window const& w)
    {
#ifdef VK_USE_PLATFORM_WIN32_KHR
        return { berry::Window::GetWin32ModuleHandle(), w.GetWin32Window() };
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
        return { w.GetX11Display(), w.GetX11Window() };
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
        return { w.GetWaylandDisplay(), w.GetWaylandSurface() };
#endif
    }

    inline static vk::Format toVkFormat(Format format)
    {
        switch (format) {
        case Format::eR8G8B8A8Unorm:
            return vk::Format::eR8G8B8A8Unorm;
        case Format::eR32G32B32A32Sfloat:
            return vk::Format::eR32G32B32A32Sfloat;
        default:
            assert(0);
            return vk::Format::eUndefined;
        }
    }
};

Vulkan::Vulkan(berry::Window const& window, module::ShaderManager& shaman)
    : impl(std::make_unique<Impl>(state, window, shaman))
{
}
Vulkan::~Vulkan() = default;

void Vulkan::SubmitFrame()
{
    impl->SubmitFrame();
}

void Vulkan::InitGUIRenderer(char const* pathFont)
{
    impl->InitGUIRenderer(pathFont);
}

void Vulkan::SetVSync(bool enable, u32 x, u32 y)
{
    impl->setVSync(enable, x, y);
}

bool Vulkan::HaveScene() const
{
    return impl->sceneOnDevice != nullptr;
}

void Vulkan::SelectNode(uint32_t nodeIndex)
{
    state.selectedNode = nodeIndex;
}

void Vulkan::ResizeSwapChain(u32 x, u32 y) const
{
    impl->ResizeSwapChain(x, y);
}

void Vulkan::ResizeRenderer(u32 x, u32 y) const
{
    impl->ResizeRenderer(x, y);
}

void Vulkan::WaitIdle() const
{
    impl->WaitIdle();
}

void Vulkan::CalculateSampleCountsForFrame() const
{
    impl->CalculateSampleCountsForFrame();
}

void Vulkan::OnRenderModeChange() const
{
    impl->RecreateRenderer();
}

void Vulkan::UploadScene(HostScene const& scene) const
{
    impl->UploadScene(scene);
}
void Vulkan::UnloadScene()
{
    impl->UnloadScene();
}
void Vulkan::UpdateTransformationMatrices(HostScene const& scene)
{
    assert(impl->sceneOnDevice);
    impl->sceneOnDevice->UpdateTransformationMatrices(scene);
}

config::BVHPipeline Vulkan::GetConfig()
{
    return state.ptComputeConfig;
}

stats::BVHPipeline Vulkan::GetStatsBuild() const
{
    return impl->GetStatsBuild();
}
stats::Trace Vulkan::GetStatsTrace() const
{
    return impl->GetStatsTrace();
}

void Vulkan::GetImageData(std::vector<glm::vec4>& result)
{
    if (!impl->renderer)
        return;
    impl->transfer.FromDevice(impl->renderer->GetBackbuffer(), result);
}

void Vulkan::SetPipelineConfiguration(config::BVHPipeline config)
{
    impl->SetPipelineConfiguration(std::move(config));
}

void Vulkan::SetCamera(input::Camera const& c)
{
    impl->SetCamera(c);
}

void Vulkan::ResetAccumulation()
{
    impl->ResetAccumulation();
}

bool Vulkan::IsRendering() const
{
    return impl->IsRendering();
}

}
