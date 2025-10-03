#include "SceneRenderer.h"

#include <imgui.h>

#include "../backend/Stats.h"

namespace module {

static f32 light_phi = glm::pi<f32>() + glm::quarter_pi<f32>();
static f32 light_theta = glm::half_pi<f32>();
static f32 light_power = glm::half_pi<f32>();
static glm::vec4 dirLight = {
    glm::sin(light_theta) * glm::cos(light_phi),
    glm::sin(light_theta) * glm::sin(light_phi),
    glm::cos(light_theta),
    light_power,
};

static std::array constexpr strRenderMode {
    "Path Tracing compute",
};

static std::array constexpr strVisMode {
    "Path tracing",
    "Bounding Volume",
    "BV intersections",
};

void SceneRenderer::ProcessGUI(State& state)
{
    // NOTE: why is this logic here?
    backend.state.oneSecondAcc += state.deltaTime;
    backend.state.oncePerSecond = false;
    if (backend.state.oneSecondAcc > 1.) {
        backend.state.oneSecondAcc -= 1.;
        backend.state.oncePerSecond = true;
    }
    backend.state.ms250Acc += state.deltaTime;
    backend.state.oncePer250ms = false;
    if (backend.state.ms250Acc > .25) {
        backend.state.ms250Acc -= .25;
        backend.state.oncePer250ms = true;
    }

    ImGui::SetNextWindowSizeConstraints(ImVec2(32, 52), ImVec2(FLT_MAX, FLT_MAX));
    ImGui::Begin("Render window");
    if ([[maybe_unused]] auto const screenResized { window.ProcessGUI(backend::gui::TextureID::renderedScene) }) {
        berry::log::debug("Resize: Render window {}x{}", window.camera.screenResolution.x, window.camera.screenResolution.y);
        backend.ResizeRenderer(window.camera.screenResolution.x, window.camera.screenResolution.y);
    }
    if (window.camera.UpdateCamera())
        backend.ResetAccumulation();

    auto const posAbsRenderWindow { ImGui::GetWindowPos() };
    auto const extentContent { ImGui::GetContentRegionAvail() };
    auto const posAbsMouse { ImGui::GetMousePos() };
    auto const posAbsButton { ImGui::GetItemRectMin() };

    glm::i32vec2 const posMouseRenderWindow { posAbsMouse.x - posAbsButton.x, posAbsMouse.y - posAbsButton.y };
    glm::i32vec2 const posRelMouse { posAbsMouse.x - posAbsRenderWindow.x, posAbsMouse.y - posAbsRenderWindow.y };
    glm::u32vec2 const extentUint { extentContent.x, extentContent.y };

    auto vMin { ImGui::GetWindowContentRegionMin() };
    auto vMax { ImGui::GetWindowContentRegionMax() };
    vMin.x += ImGui::GetWindowPos().x;
    vMin.y += ImGui::GetWindowPos().y;
    vMax.x += ImGui::GetWindowPos().x;
    vMax.y += ImGui::GetWindowPos().y;

    if (backend.IsRendering()) {
        ImGui::GetWindowDrawList()->AddRect(vMin, vMax, IM_COL32(255, 0, 0, 255));
    }

    if (state.statusBarFade > 0.f) {
        ImVec2 posSymbol { (vMin.x + vMax.x) * .5f, (vMin.y + vMax.y) * .5f };
        ImVec2 posSymbolUpper { posSymbol.x, posSymbol.y - 60.f };
        ImVec2 posText { posSymbol.x - 55.f, posSymbol.y + 35.f };
        auto size { (std::sin(static_cast<float>(state.time) * 2.5f) + 1.f) * 8.f + 30.f };
        auto color { ImGui::GetStyleColorVec4(ImGuiCol_PlotHistogram) };
        color.w *= state.statusBarFade;
        auto colorU32 { ImGui::GetColorU32(color) };

        ImGui::GetWindowDrawList()->PathArcTo(posSymbolUpper, size - 0.5f, glm::pi<float>() * 1.9f, glm::pi<float>() * 3.1f, 4);
        ImGui::GetWindowDrawList()->PathStroke(colorU32, ImDrawFlags_Closed, size * .2f);
        ImGui::GetWindowDrawList()->PathArcTo(posSymbol, size - 0.5f, glm::pi<float>() * 0.9f, glm::pi<float>() * 2.1f, 4);
        ImGui::GetWindowDrawList()->PathStroke(colorU32, ImDrawFlags_Closed, size * .2f);
        ImGui::GetWindowDrawList()->AddText(nullptr, 26.f, posText, colorU32, "loading..");
    }

    ImGui::SetItemAllowOverlap();
    ImGui::End();

    bool renderModeChanged { false };
    if (ImGui::IsKeyPressed(ImGuiKey_F)) {
        backend.state.selectedRenderer = backend::vulkan::State::Renderer((backend.state.selectedRenderer.as_ut() + 1) % csize<int>(strRenderMode));
        renderModeChanged = true;
        backend.ResetAccumulation();
    }

    ImGui::Begin("Scene renderer");

    ImGui::Text("Render resolution : %.0u, %.0u", window.camera.screenResolution.x, window.camera.screenResolution.y);

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.f);
    // NOTE: 16 and 36 are border/window padding constants, perhaps should be queried
    if (ImGui::Button("256x256"))
        ImGui::SetWindowSize("Render window", ImVec2(256 + 16, 256 + 36));
    ImGui::SameLine();
    if (ImGui::Button("512x512"))
        ImGui::SetWindowSize("Render window", ImVec2(512 + 16, 512 + 36));
    ImGui::SameLine();
    if (ImGui::Button("1024x768"))
        ImGui::SetWindowSize("Render window", ImVec2(1024 + 16, 768 + 36));
    ImGui::SameLine();
    if (ImGui::Button("1920x1080"))
        ImGui::SetWindowSize("Render window", ImVec2(1920 + 16, 1080 + 36));
    ImGui::PopStyleVar();

    ImGui::Separator();
    ImGuiIO& io = ImGui::GetIO();
    if (ImGui::Checkbox("vSync", &backend.state.vSync))
        backend.SetVSync(backend.state.vSync, state.windowSize.x, state.windowSize.y);
    ImGui::SameLine();
    ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    if (ImGui::Combo("Render Mode", &backend.state.selectedRenderer.as_ut(), strRenderMode.data(), static_cast<int>(strRenderMode.size()))) {
        renderModeChanged = true;
        backend.ResetAccumulation();
    }
    if (renderModeChanged)
        backend.OnRenderModeChange();

    ImGui::Separator();
    switch (backend.state.selectedRenderer) {
    case backend::vulkan::State::Renderer::ePathTracingCompute:
        guiRayTracerShared();
        ImGui::Separator();
        guiPathTracerCompute();
        break;
    }
    ImGui::Separator();

    ImGui::End();
    backend.ProcessGUI();
}

void SceneRenderer::guiRayTracerShared()
{
    auto const sppButton { [this](u32 spp) {
        if (ImGui::Button(std::to_string(spp).c_str())) {
            backend.state.samplesPerPixel = spp;
            backend.ResetAccumulation();
        }
    } };
    ImGui::Text("Samples per pixel: ");
    sppButton(1);
    ImGui::SameLine();
    sppButton(8);
    ImGui::SameLine();
    sppButton(16);
    ImGui::SameLine();
    sppButton(32);
    ImGui::SameLine();
    sppButton(64);
    ImGui::SameLine();
    sppButton(128);
    ImGui::SameLine();
    sppButton(512);
    ImGui::SameLine();
    sppButton(1024);
    ImGui::SameLine();
    sppButton(2048);
    ImGui::SameLine();
    if (ImGui::Button("inf")) {
        backend.state.samplesPerPixel = std::numeric_limits<u32>::max();
        backend.ResetAccumulation();
    }
    if (gui::u32Slider("##SampleCounterSlider", &backend.state.samplesPerPixel, 1, 65535, "%u", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic))
        backend.ResetAccumulation();
    ImGui::Text("Computed samples: %u", std::max(1u, backend.state.samplesComputed));
    if (gui::u32Slider("PT depth", &backend.state.ptDepth, 0, 7, "%u", ImGuiSliderFlags_AlwaysClamp)) {
        backend.ResetAccumulation();
    }

    ImGui::Separator();
    ImGui::Text("dir. light: %.2f %.2f %.2f %.2f", dirLight.x, dirLight.y, dirLight.z, dirLight.w);
    bool updateLight = false;
    if (ImGui::DragFloat("phi   (0..2pi)", &light_phi, .01f, 0.f, 2.f * glm::pi<f32>()))
        updateLight = true;
    if (ImGui::DragFloat("theta (0..pi)", &light_theta, .005f, 0.f, glm::pi<f32>()))
        updateLight = true;
    if (ImGui::DragFloat("power", &light_power, .005f, 0.f, 10.f, nullptr, ImGuiSliderFlags_Logarithmic))
        updateLight = true;

    if (updateLight) {
        auto const sinTheta = glm::sin(light_theta);
        dirLight.x = sinTheta * glm::cos(light_phi);
        dirLight.y = sinTheta * glm::sin(light_phi);
        dirLight.z = glm::cos(light_theta);
        dirLight.w = light_power;
        backend.ResetAccumulation();
    }
    if (backend.state.selectedVisMode != backend::vulkan::State::VisMode::eBVIntersections)
        memcpy(&backend.state.dirLight, &dirLight, 16);
}

void SceneRenderer::guiPathTracerCompute()
{
    if (ImGui::TreeNodeEx("Output", ImGuiTreeNodeFlags_DefaultOpen)) {
        static u32 depthViz { 1 };
        static i32 heatMapRangeMin { 0 };
        static i32 heatMapRangeMax { 250 };
        static u32 depthCache { 7 };

        for (int i { 0 }; i < csize<i32>(strVisMode); ++i) {
            if (ImGui::Selectable(strVisMode[i], backend.state.selectedVisMode.as_ut() == i)) {
                if (backend.state.selectedVisMode.as_ut() == i)
                    continue;

                backend.state.selectedVisMode = backend::vulkan::State::VisMode(i);
                backend.ResetAccumulation();

                if (backend.state.selectedVisMode == backend::vulkan::State::VisMode::eBVIntersections) {
                    backend.state.dirLight[0] = static_cast<f32>(heatMapRangeMin);
                    backend.state.dirLight[1] = static_cast<f32>(heatMapRangeMax);
                    depthCache = backend.state.ptDepth;
                    backend.state.ptDepth = 0;
                } else {
                    backend.state.ptDepth = depthCache;
                    memcpy(&backend.state.dirLight, &dirLight, 16);
                }
            }
        }
        ImGui::Separator();
        auto cfg { backend.GetConfig() };
        bool configChanged { false };
        switch (backend.state.selectedVisMode) {
        case backend::vulkan::State::VisMode::ePathTracing:
            break;
        case backend::vulkan::State::VisMode::eBVVisualization:
            if (gui::u32Slider("##CurrentBVHLevel", &cfg.tracer.bvDepth, 1, 256, "%u", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic))
                configChanged = true;
            ImGui::SameLine();
            ImGui::BeginDisabled(cfg.tracer.bvDepth == 1);
            if (ImGui::Button("-")) {
                cfg.tracer.bvDepth--;
                configChanged = true;
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::BeginDisabled(depthViz == 256);
            if (ImGui::Button("+")) {
                cfg.tracer.bvDepth++;
                configChanged = true;
            }
            ImGui::EndDisabled();
            if (ImGui::Checkbox("Render triangles", &cfg.tracer.bvRenderTriangles))
                configChanged = true;

            break;
        case backend::vulkan::State::VisMode::eBVIntersections: {

            if (ImGui::DragIntRange2("Heat map range", &heatMapRangeMin, &heatMapRangeMax, 1, 0, 1024, "Low: < %d", "High: > %d")) {
                // NOTE: repurposed light field
                if (heatMapRangeMin == heatMapRangeMax)
                    heatMapRangeMax++;
                backend.state.dirLight[0] = static_cast<f32>(heatMapRangeMin);
                backend.state.dirLight[1] = static_cast<f32>(heatMapRangeMax);
                configChanged = true;
            }
            static bool showBVs { true };
            static bool showTris { true };
            if (ImGui::Checkbox("Bounding volumes", &showBVs))
                backend.ResetAccumulation();
            if (ImGui::Checkbox("Triangles", &showTris))
                backend.ResetAccumulation();
            backend.state.dirLight[2] = static_cast<f32>(showBVs);
            backend.state.dirLight[3] = static_cast<f32>(showTris);
            break;
        }
        default:
            break;
        }
        if (configChanged) {
            backend.SetPipelineConfiguration(cfg);

            if (backend.state.selectedVisMode != backend::vulkan::State::VisMode::eBVIntersections)
                memcpy(&backend.state.dirLight, &dirLight, 16);
        }
        ImGui::TreePop();
    }
    ImGui::Separator();

    if (ImGui::TreeNodeEx("BVH stats", ImGuiTreeNodeFlags_DefaultOpen)) {
        static f32 pMRps { 0.f };
        static f32 sMRps { 0.f };
        static f32 buildTime { 0.f };

        static u64 pRayCount { 0 };
        static f32 pTraceTimeMs { 0.f };
        static u64 sRayCount { 0 };
        static f32 sTraceTimeMs { 0.f };

        static std::array<f32, 7> sMRps_perLevel {};
        static std::array<u64, 7> sRayCount_perLevel {};
        static std::array<f32, 7> sTraceTimeMs_perLevel {};

        auto const bStats { backend.GetStatsBuild() };
        auto const tStats { backend.GetStatsTrace() };
        pRayCount += tStats.data[0].rayCount;
        pTraceTimeMs += tStats.data[0].traceTimeMs;
        for (int i = 1; i < 8; i++) {
            sRayCount += tStats.data[i].rayCount;
            sTraceTimeMs += tStats.data[i].traceTimeMs;

            sRayCount_perLevel[i - 1] += tStats.data[i].rayCount;
            sTraceTimeMs_perLevel[i - 1] += tStats.data[i].traceTimeMs;
        }
        if (backend.state.oncePer250ms) {
            pMRps = (static_cast<f32>(pRayCount) * 1e-6f) / (pTraceTimeMs * 1e-3f);
            sMRps = (static_cast<f32>(sRayCount) * 1e-6f) / (sTraceTimeMs * 1e-3f);
            pRayCount = 0;
            pTraceTimeMs = 0.f;
            sRayCount = 0;
            sTraceTimeMs = 0.f;

            for (int i = 0; i < 7; i++) {
                sMRps_perLevel[i] = (static_cast<f32>(sRayCount_perLevel[i]) * 1e-6f) / (sTraceTimeMs_perLevel[i] * 1e-3f);
                sRayCount_perLevel[i] = 0;
                sTraceTimeMs_perLevel[i] = 0.f;
            }
        }
        buildTime = bStats.plocpp.timeTotal + bStats.collapsing.timeTotal + bStats.transformation.timeTotal + bStats.rearrangement.timeTotal;

        ImGui::Text("    Primary:  ~%s Mrps", fmt::format("{:>8.2f}", pMRps).c_str());
        ImGui::Text("  Secondary:  ~%s Mrps", fmt::format("{:>8.2f}", sMRps).c_str());
        if (ImGui::TreeNodeEx("Secondary per bounce")) {
            for (int i = 0; i < 7; i++)
                ImGui::Text("     Sec. %d:  ~%s Mrps", i, fmt::format("{:>8.2f}", sMRps_perLevel[i]).c_str());
            ImGui::TreePop();
        }
        ImGui::Separator();
        ImGui::Text("  Full #Nodes:    %s", fmt::format("{:14L}", bStats.plocpp.nodeCountTotal).c_str());
        ImGui::Text("  Full area rel. i:      %s", fmt::format("{:>8.2f}", bStats.plocpp.saIntersect).c_str());
        ImGui::Text("  Full area rel. t:      %s", fmt::format("{:>8.2f}", bStats.plocpp.saTraverse).c_str());
        ImGui::Text("  Full SAH cost:        %s", fmt::format("{:>8.2f}", bStats.plocpp.costTotal).c_str());
        ImGui::Separator();
        ImGui::Text("  Collapsed #N:   %s", fmt::format("{:14L}", bStats.collapsing.nodeCountTotal).c_str());
        ImGui::Text("  Collapsed area rel. i: %s", fmt::format("{:>8.2f}", bStats.collapsing.saIntersect).c_str());
        ImGui::Text("  Collapsed area rel. t: %s", fmt::format("{:>8.2f}", bStats.collapsing.saTraverse).c_str());
        ImGui::Text("  Collapsed SAH cost:   %s", fmt::format("{:>8.2f}", bStats.collapsing.costTotal).c_str());
        if (bStats.transformation.costTotal > 0.f) {
            ImGui::Separator();
            ImGui::Text("  Transformed #N:   %s", fmt::format("{:14L}", bStats.transformation.nodeCountTotal).c_str());
            ImGui::Text("  Transformed area rel. i: %s", fmt::format("{:>8.2f}", bStats.transformation.saIntersect).c_str());
            ImGui::Text("  Transformed area rel. t: %s", fmt::format("{:>8.2f}", bStats.transformation.saTraverse).c_str());
            ImGui::Text("  Transformed SAH cost:   %s", fmt::format("{:>8.2f}", bStats.transformation.costTotal).c_str());
        }
        ImGui::Separator();
        ImGui::Text("  Arranged #N:   %s", fmt::format("{:14L}", bStats.rearrangement.nodeCountTotal).c_str());
        ImGui::Text("  Arranged area rel. i: %s", fmt::format("{:>8.2f}", bStats.rearrangement.saIntersect).c_str());
        ImGui::Text("  Arranged area rel. t: %s", fmt::format("{:>8.2f}", bStats.rearrangement.saTraverse).c_str());
        ImGui::Text("  Arranged SAH cost:   %s", fmt::format("{:>8.2f}", bStats.rearrangement.costTotal).c_str());
        ImGui::Separator();
        ImGui::Text("  Construction GPU: %s ms", fmt::format("{:>8.2f}", buildTime).c_str());

        ImGui::TreePop();
    }
    ImGui::Separator();
}

}
