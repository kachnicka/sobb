#include "Vulkan.h"

#include "../../ApplicationState.h"
#include <imgui.h>

namespace backend::vulkan {

[[maybe_unused]] static void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void Vulkan::ProcessGUI()
{
}

}
