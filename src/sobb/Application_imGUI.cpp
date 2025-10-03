#include "Application.h"

#include "imgui_internal.h"
#include "scene/Serialization.h"

void Application::ProcessGUI()
{
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_AutoHideTabBar);

    if (ImGui::IsKeyPressed(ImGuiKey_Z))
        state.showImguiDemo = !state.showImguiDemo;

    if (state.showImguiDemo)
        ImGui::ShowDemoWindow(&state.showImguiDemo);

    ImGui::Begin("SOBB");

    ImGui::Checkbox("Shader hot reload", &state.shaderHotReload);

    static auto const pathOb { directory.res / "scene" / "binary" };
    std::filesystem::create_directories(pathOb);
    ImGui::Text("path: %s", pathOb.generic_string().c_str());
    if (ImGui::Button("serialize to binary"))
        scene::serialize(*scenes.back(), pathOb);

    static auto const pathImg { directory.res / "image" };
    std::filesystem::create_directories(pathImg);
    static char buf[48] = "";
    ImGui::Text("path: %s", pathImg.generic_string().c_str());
    ImGui::InputText("Image name [48]", buf, IM_ARRAYSIZE(buf));
    if (ImGui::Button("Save image")) {
        auto path { pathImg / buf };
        std::filesystem::create_directories(path.parent_path());
        saveRenderWindowAsImage(path);
    }
    ImGui::End();

    if (ImGui::BeginViewportSideBar("##MainStatusBar", nullptr, ImGuiDir_Down, ImGui::GetFrameHeight(), ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar)) {
        if (ImGui::BeginMenuBar()) {
            if (state.statusBarFade > 0.f) {
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, state.statusBarFade);
                ImGui::ProgressBar(state.progress, ImVec2(0.0f, 0.0f));
                ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
                if (!asyncProcessing.scenes.empty())
                    ImGui::Text("loading...");
                else {
                    ImGui::Text("done.");
                    state.statusBarFade -= 0.02f;
                }
                ImGui::PopStyleVar();
            }
            ImGui::EndMenuBar();
        }
    }
    ImGui::End();
}
