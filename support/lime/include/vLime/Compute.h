#pragma once

#include <string_view>
#include <vLime/Reflection.h>
#include <vLime/Vulkan.h>
#include <vLime/types.h>

namespace lime {
[[nodiscard]] constexpr static u32 divCeil(u32 a, u32 b)
{
    return (a + b - 1) / b;
}

class PipelineCompute {
    vk::UniquePipeline pipeline;

    u32 version { static_cast<u32>(-1) };
    std::string_view shaderName;
    vk::SpecializationInfo sInfo;

public:
    PipelineLayout layout;

    [[nodiscard]] bool isValid() const
    {
        return !shaderName.empty();
    }

    [[nodiscard]] vk::Pipeline get() const
    {
        return pipeline.get();
    }

    [[nodiscard]] vk::PipelineLayout getLayout() const
    {
        return layout.pipeline.get();
    }

    PipelineCompute() = default;
    PipelineCompute(vk::Device d, ShaderCache& cache, std::string_view shaderName, vk::SpecializationInfo const& specializationInfo = {})
        : shaderName(shaderName)
        , sInfo(specializationInfo)
    {
        Update(d, cache);
    }

    bool Update(vk::Device d, ShaderCache& cache)
    {
        if (!isValid())
            return false;

        auto const& shader = cache.LoadShader(shaderName);
        if (shader.version == version)
            return false;

        version = shader.version;
        PipelineLayoutBuilder layoutBuilder;
        layoutBuilder.SetLayoutForShader(shader);
        layout = layoutBuilder.Build(d);

        vk::ComputePipelineCreateInfo cInfo {
            .stage = shader.GetStageCreateInfo(),
            .layout = layout.pipeline.get(),
        };

        // NOTE: force subgroup (warp) size 32
        // should check size properties to validate 32 size availability
        vk::PipelineShaderStageRequiredSubgroupSizeCreateInfo sSizeInfo {
            .requiredSubgroupSize = 32,
        };
        cInfo.stage.pNext = &sSizeInfo;

        if (sInfo.mapEntryCount > 0)
            cInfo.stage.pSpecializationInfo = &sInfo;

        pipeline = check(d.createComputePipelineUnique(cache.getPipelineCache(), cInfo));
        return true;
    }
};

}
