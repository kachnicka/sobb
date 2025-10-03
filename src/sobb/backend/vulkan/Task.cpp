#include "Task.h"
#include <vulkan/vulkan_enums.hpp>

namespace backend::vulkan::task_old_style {

void ImGui::scheduleRgTask(lime::rg::Graph& rg)
{
    task = rg.AddTask<lime::rg::Rasterization>();

    rg.GetTask(task).RegisterExecutionCallback([this](vk::CommandBuffer commandBuffer, lime::rg::RasterizationInfo const& info) {
        renderer.recordCommands(commandBuffer, info, scene);
    });

    if (!rg.isValid(imgRendered))
        imgRendered = rg.AddResource(vk::Format::eR8G8B8A8Unorm);
    rg.SetTaskResourceOperation(task, imgRendered, lime::rg::Resource::OperationFlagBits::eSampledFrom);

    if (!rg.isValid(imgTarget))
        imgTarget = rg.AddResource(vk::Format::eR8G8B8A8Unorm);
    rg.SetTaskResourceOperation(task, imgTarget, lime::rg::Resource::OperationFlagBits::eStored);
}

}
