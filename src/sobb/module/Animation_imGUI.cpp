#include "Animation.h"

#include "../core/GUI.h"

namespace module {

void Animation::ProcessGUI(State& _state)
{
    static_cast<void>(_state);

    ImGui::Begin("Animation");
    ImGui::BeginDisabled(animation.running);

    if (ImGui::InputInt("SPP", &animation.spp))
        animation.spp = std::max(1, animation.spp);
    if (ImGui::InputInt("BVH min depth", &animation.minDepth)) {
        animation.minDepth = std::max(0, animation.minDepth);
        animation.minDepth = std::min(99, animation.minDepth);
    }
    if (ImGui::InputInt("BVH max depth", &animation.maxDepth)) {
        animation.maxDepth = std::max(0, animation.maxDepth);
        animation.maxDepth = std::min(99, animation.maxDepth);
    }
    ImGui::InputInt("Frames per BVH level", &animation.framesPerLevel);
    ImGui::InputInt("Frames for level trans.", &animation.levelTransitionFrames);
    ImGui::Separator();
    ImGui::InputFloat("Speed", &animation.speed);
    ImGui::InputFloat("Zoom", &animation.zoom);
    ImGui::InputFloat("Pan X", &animation.panX);
    ImGui::InputFloat("Pan Y", &animation.panY);
    ImGui::Checkbox("Save Images", &animation.saveImages);

    ImGui::InputText("Name", &animation.name_TMP);

    if (ImGui::Button("Record Animation"))
        animation.running = true;
    ImGui::EndDisabled();

    if (animation.running) {
        ImGui::SameLine();
        if (ImGui::Button("STOP")) {
            animation.frame = 0;
            animation.running = false;
        }
    }

    ImGui::End();
}

}
