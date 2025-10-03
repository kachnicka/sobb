#pragma once

#include <limits>
#include <vLime/CommandPool.h>

namespace lime {

class Frame {
    vk::Device d;
    vk::Queue q;
    vk::UniqueCommandPool pool;
    u64 currentFrameId { 0 };

public:
    vk::CommandBuffer commandBuffer;
    vk::UniqueFence renderFinishedFence;
    vk::UniqueSemaphore imageAvailableSemaphore;
    std::vector<vk::UniqueSemaphore> renderFinishedSemaphores;

public:
    Frame(vk::Device d, lime::Queue queue, u32 swapChainImageCount)
        : d(d)
        , q(queue.q)
        , renderFinishedFence(FenceFactory(d, vk::FenceCreateFlagBits::eSignaled))
        , imageAvailableSemaphore(SemaphoreFactory(d))
        , renderFinishedSemaphores(createRenderFinishedSemaphores(d, swapChainImageCount))
    {
        vk::CommandPoolCreateInfo poolCreateInfo {
            .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = queue.queueFamilyIndex,
        };
        pool = check(d.createCommandPoolUnique(poolCreateInfo));

        vk::CommandBufferAllocateInfo allocInfo {
            .commandPool = pool.get(),
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1,
        };
        check(d.allocateCommandBuffers(&allocInfo, &commandBuffer));
    }

    void ResetFrameCounter()
    {
        currentFrameId = 0u;
    }

    [[nodiscard]] u64 CurrentFrameId() const
    {
        return currentFrameId;
    }

    void Wait() const
    {
        check(d.waitForFences(1, &renderFinishedFence.get(), vk::True, std::numeric_limits<u64>::max()));
    }

    [[nodiscard]] std::pair<u64, vk::CommandBuffer> ResetAndBeginCommandsRecording() const
    {
        commandBuffer.reset(vk::CommandBufferResetFlags());

        vk::CommandBufferBeginInfo beginInfo;
        check(commandBuffer.begin(&beginInfo));

        return { currentFrameId, commandBuffer };
    }

    void EndCommandsRecording() const
    {
        check(commandBuffer.end());
    }

    std::vector<vk::Semaphore> SubmitCommands(u32 swapChainImageId, bool waitSemaphores = true, bool signalSemaphores = true)
    {
        if (swapChainImageId == INVALID_ID)
            return {};

        std::vector<vk::Semaphore> result;

        std::vector<vk::PipelineStageFlags> waitStages = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
        vk::SubmitInfo submitInfo {
            .pWaitDstStageMask = waitStages.data(),
            .commandBufferCount = 1,
            .pCommandBuffers = &commandBuffer,
        };

        if (waitSemaphores) {
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = &imageAvailableSemaphore.get();
        }
        if (signalSemaphores) {
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = &renderFinishedSemaphores[swapChainImageId].get();
            result.push_back(renderFinishedSemaphores[swapChainImageId].get());
        }

        check(d.resetFences(1, &renderFinishedFence.get()));
        check(q.submit(submitInfo, renderFinishedFence.get()));
        currentFrameId++;

        return result;
    }

private:
    static std::vector<vk::UniqueSemaphore> createRenderFinishedSemaphores(vk::Device d, u32 swapChainImageCount)
    {
        std::vector<vk::UniqueSemaphore> result;
        result.reserve(swapChainImageCount);
        for (u32 i = 0; i < swapChainImageCount; i++)
            result.emplace_back(SemaphoreFactory(d));
        return result;
    }
};

}
