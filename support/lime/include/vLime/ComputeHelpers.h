#pragma once

#include <vLime/Memory.h>
#include <vLime/types.h>

namespace lime::compute {

inline static vk::MemoryBarrier barrierBufferWrite {
    .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
    .dstAccessMask = vk::AccessFlagBits::eShaderRead,
};
inline static vk::MemoryBarrier barrierBufferRead {
    .srcAccessMask = vk::AccessFlagBits::eShaderWrite,
    .dstAccessMask = vk::AccessFlagBits::eTransferRead,
};
inline static vk::MemoryBarrier barrierCompute {
    .srcAccessMask = vk::AccessFlagBits::eShaderWrite,
    .dstAccessMask = vk::AccessFlagBits::eShaderRead,
};

inline static void pBarrierTransferWrite(vk::CommandBuffer commandBuffer)
{
    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags(), barrierBufferWrite, nullptr, nullptr);
}

inline static void pBarrierTransferRead(vk::CommandBuffer commandBuffer)
{
    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags(), barrierBufferRead, nullptr, nullptr);
}

inline static void pBarrierCompute(vk::CommandBuffer commandBuffer)
{
    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags(), barrierCompute, nullptr, nullptr);
}

inline static void fillZeros(vk::CommandBuffer commandBuffer, lime::Buffer::Detail const& buffer)
{
    commandBuffer.fillBuffer(buffer.get(), 0, buffer.getSizeInBytes(), 0);
}

}
