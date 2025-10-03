#include "Benchmark.h"

#include "../Application.h"
#include "../core/ConfigFiles.h"

static std::string to_string(backend::config::BV bv)
{
    switch (bv) {
    case backend::config::BV::eNone:
        return "none";
    case backend::config::BV::eAABB:
        return "aabb";
    case backend::config::BV::eDOP14:
        return "dop14";
    case backend::config::BV::eDOP14split:
        return "dop14 split";
    case backend::config::BV::eOBB:
        return "obb";
    case backend::config::BV::eSOBB_d:
        return "sobb d";
    case backend::config::BV::eSOBB_d32:
        return "sobb d32";
    case backend::config::BV::eSOBB_d48:
        return "sobb d48";
    case backend::config::BV::eSOBB_d64:
        return "sobb d64";
    case backend::config::BV::eSOBB_i32:
        return "sobb i32";
    case backend::config::BV::eSOBB_i48:
        return "sobb i48";
    case backend::config::BV::eSOBB_i64:
        return "sobb i64";
    }
    return "unknown";
}

static std::string to_string(backend::config::NodeLayout layout)
{
    switch (layout) {
    case backend::config::NodeLayout::eDefault:
        return "bvh2 default";
    case backend::config::NodeLayout::eBVH2:
        return "bvh2";
    }
    return "unknown";
}

static std::string to_string(backend::config::InitialClusters ic)
{
    switch (ic) {
    case backend::config::InitialClusters::eTriangles:
        return "triangles";
    }
    return "unknown";
}

namespace module {

void Benchmark::ProcessGUI(State& state)
{
    ImGui::Begin("Benchmark");
    ImGui::BeginDisabled(rt.bRunning);

    if (ImGui::Button("Reload Config"))
        LoadConfig();

    ImGui::SameLine();
    if (ImGui::Button("Run"))
        SetupBenchmarkRun(state);

    ImGui::BeginChild("b. list", ImVec2(120, ImGui::GetContentRegionAvail().y), true, ImGuiWindowFlags_HorizontalScrollbar);
    static int bShowPreview { 0 };
    for (int i { 0 }; i < csize<int>(bPipelines); ++i) {
        if (ImGui::Selectable(bPipelines[i].name.c_str(), rt.currentPipeline == i, ImGuiSelectableFlags_AllowDoubleClick)) {
            bShowPreview = i;
            if (ImGui::IsMouseDoubleClicked(0)) {
                rt.currentPipeline = i;
                backend.SetPipelineConfiguration(bPipelines[i]);
            }
        }
    }
    ImGui::NewLine();
    auto& scenes { app.configFiles.GetScenes() };
    for (int i { 0 }; i < csize<int>(scenes); ++i) {
        if (ImGui::Selectable(scenes[i].name.c_str(), rt.currentScene == i, ImGuiSelectableFlags_AllowDoubleClick)) {
            if (ImGui::IsMouseDoubleClicked(0)) {
                rt.currentScene = i;
                state.sceneToLoad = scenes[i];
            }
        }
    }
    ImGui::EndChild();

    char buf[32];
    auto const columnWidth { (ImGui::GetContentRegionAvail().x - 120) - 8 };
    static ImGuiTableFlags tableFlags { ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody };
    static auto const printConfigValue { [&buf](char const* label, char const* format, auto value) {
        ImGui::TableNextColumn();
        std::snprintf(buf, sizeof(buf), "%s", label);
        ImGui::TextUnformatted(buf);
        ImGui::TableNextColumn();
        std::snprintf(buf, sizeof(buf), format, value);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(buf).x);
        ImGui::TextUnformatted(buf);
    } };

    ImGui::SameLine();
    ImGui::BeginChild("Preview", ImVec2(columnWidth, ImGui::GetContentRegionAvail().y), true);
    if (ImGui::BeginTable("##preview plocpp", 1, tableFlags)) {
        ImGui::TableSetupColumn("PLOCpp", ImGuiTableColumnFlags_NoHide);
        ImGui::TableHeadersRow();

        printConfigValue("b. volume", "%s", to_string(bPipelines[bShowPreview].plocpp.bv).c_str());
        printConfigValue("s. filling", "%s", "morton32");
        printConfigValue("i. clusts", "%s", to_string(bPipelines[bShowPreview].plocpp.ic).c_str());
        printConfigValue("radius", "%u", bPipelines[bShowPreview].plocpp.radius);

        ImGui::EndTable();
    }
    if (ImGui::BeginTable("##preview collapsing", 1, tableFlags)) {
        ImGui::TableSetupColumn("Collapsing", ImGuiTableColumnFlags_NoHide);
        ImGui::TableHeadersRow();

        printConfigValue("b. volume", "%s", to_string(bPipelines[bShowPreview].collapsing.bv).c_str());
        printConfigValue("max leaf size", "%u", bPipelines[bShowPreview].collapsing.maxLeafSize);
        printConfigValue("c_t", "%.1f", bPipelines[bShowPreview].collapsing.c_t);
        printConfigValue("c_i", "%.1f", bPipelines[bShowPreview].collapsing.c_i);

        ImGui::EndTable();
    }
    if (ImGui::BeginTable("##preview transformation", 1, tableFlags)) {
        ImGui::TableSetupColumn("Transformation", ImGuiTableColumnFlags_NoHide);
        ImGui::TableHeadersRow();

        printConfigValue("b. volume", "%s", to_string(bPipelines[bShowPreview].transformation.bv).c_str());

        ImGui::EndTable();
    }
    if (ImGui::BeginTable("##preview rearrangement", 1, tableFlags)) {
        ImGui::TableSetupColumn("Rearrangement", ImGuiTableColumnFlags_NoHide);
        ImGui::TableHeadersRow();

        printConfigValue("b. volume", "%s", to_string(bPipelines[bShowPreview].rearrangement.bv).c_str());
        printConfigValue("layout", "%s", to_string(bPipelines[bShowPreview].rearrangement.layout).c_str());

        ImGui::EndTable();
    }
    if (ImGui::BeginTable("##preview tracer", 1, tableFlags)) {
        ImGui::TableSetupColumn("Tracer", ImGuiTableColumnFlags_NoHide);
        ImGui::TableHeadersRow();

        printConfigValue("b. volume", "%s", to_string(bPipelines[bShowPreview].tracer.bv).c_str());
        printConfigValue("#p wg", "%u", bPipelines[bShowPreview].tracer.rPrimary.workgroupCount);
        printConfigValue("#p warps per wg", "%u", bPipelines[bShowPreview].tracer.rPrimary.warpsPerWorkgroup);
        printConfigValue("#s wg", "%u", bPipelines[bShowPreview].tracer.rSecondary.workgroupCount);
        printConfigValue("#s warps per wg", "%u", bPipelines[bShowPreview].tracer.rSecondary.warpsPerWorkgroup);

        ImGui::EndTable();
    }
    if (ImGui::BeginTable("##preview stats", 1, tableFlags)) {
        ImGui::TableSetupColumn("Stats", ImGuiTableColumnFlags_NoHide);
        ImGui::TableHeadersRow();

        // printConfigValue("b. volume", "%s", to_string(bPipelines[bShowPreview].stats.bv).c_str());
        printConfigValue("c_t", "%.1f", bPipelines[bShowPreview].stats.c_t);
        printConfigValue("c_i", "%.1f", bPipelines[bShowPreview].stats.c_i);

        ImGui::EndTable();
    }
    ImGui::EndChild();

    ImGui::EndDisabled();
    ImGui::End();
}

}
