#pragma once

#include <functional>
#include <string_view>
#include <unordered_map>
#include <vLime/PipelineBuilder.h>
#include <vLime/Util.h>
#include <vLime/Vulkan.h>
#include <vLime/types.h>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>

namespace lime {

struct Shader {
    struct LayoutReflection {
        std::vector<std::pair<u32, vk::DescriptorSetLayoutBinding>> bindings;
        std::vector<vk::PushConstantRange> pcRanges;
    };

    vk::ShaderStageFlagBits stage;
    vk::UniqueShaderModule module;
    LayoutReflection layoutReflection;
    u32 version { 0 };

    Shader(vk::Device d, vk::ShaderStageFlagBits stage, std::vector<u32> const& spv)
        : stage(stage)
    {
        Update(d, spv);
    }

    [[nodiscard]] vk::PipelineShaderStageCreateInfo GetStageCreateInfo(char const* entryPoint = "main") const
    {
        return {
            .stage = stage,
            .module = module.get(),
            .pName = entryPoint,
        };
    }

    void Update(vk::Device d, std::vector<u32> const& spv)
    {
        layoutReflection = reflect(spv, stage);
        vk::ShaderModuleCreateInfo cInfo {
            .codeSize = spv.size() * sizeof(u32),
            .pCode = spv.data()
        };
        module = check(d.createShaderModuleUnique(cInfo));
        version++;
    }

private:
    Shader::LayoutReflection reflect(std::vector<u32> spv, vk::ShaderStageFlagBits shaderStage);
};

class ShaderCache {
    vk::Device d;
    vk::UniquePipelineCache pCache;
    std::unordered_map<std::string, Shader> cache;

    std::function<void(std::string_view, std::vector<u32>&, std::function<void(std::string_view, std::vector<u32>)>)> spvLoadFunc;
    std::function<void(std::string_view)> spvUnloadFunc;
    std::unordered_map<std::string, std::vector<u32>> spvUpdate;

public:
    ShaderCache(vk::Device d,
        std::function<void(std::string_view, std::vector<u32>&, std::function<void(std::string_view, std::vector<u32>)>)> spvLoadFunc,
        std::function<void(std::string_view)> spvUnloadFunc)
        : d(d)
        , pCache(createPipelineCache())
        , spvLoadFunc(std::move(spvLoadFunc))
        , spvUnloadFunc(std::move(spvUnloadFunc))
    {
    }

    [[nodiscard]] vk::PipelineShaderStageCreateInfo getShaderCreateInfo(std::string_view shaderName, char const* entryPoint = "main") const
    {
        std::string key { shaderName };
        if (auto const m { cache.find(key) }; m != cache.cend())
            return m->second.GetStageCreateInfo(entryPoint);
        return {};
    }

    [[nodiscard]] vk::PipelineCache getPipelineCache() const
    {
        return pCache.get();
    }

    Shader const& LoadShader(std::string_view shaderName)
    {
        std::string key { shaderName };
        if (auto const it { cache.find(key) }; it != cache.cend())
            return it->second;

        std::vector<u32> spv;
        spvLoadFunc(shaderName, spv, [this](auto name, auto spv) {
            std::string key { name };
            spvUpdate[key] = std::move(spv);
        });
        auto const c = cache.insert_or_assign(key, Shader { d, stageFlagFromFileName(shaderName), spv });
        return c.first->second;
    }

    bool CheckForHotReload()
    {
        bool result { !spvUpdate.empty() };
        for (auto& [name, spv] : spvUpdate)
            cache.at(name).Update(d, spv);
        spvUpdate.clear();
        return result;
    }

private:
    [[nodiscard]] vk::UniquePipelineCache createPipelineCache() const
    {
        return check(d.createPipelineCacheUnique({}));
    }
    [[nodiscard]] vk::ShaderStageFlagBits stageFlagFromFileName(std::string_view name);
};

struct PipelineLayout {
    vk::UniquePipelineLayout pipeline;
    std::vector<vk::UniqueDescriptorSetLayout> dSet;
};

class PipelineLayoutBuilder {
    std::vector<std::vector<vk::DescriptorSetLayoutBinding>> dsLayouts;
    std::unordered_map<vk::ShaderStageFlagBits, std::vector<vk::PushConstantRange>> pcRanges;

public:
    void SetLayoutForShader(Shader const& shader);
    void AddDescriptorSetBinding(u32 dSetNum, vk::DescriptorSetLayoutBinding const& dsBinding);

    [[nodiscard]] PipelineLayout Build(vk::Device d) const;

    void AddPushConstantRange(vk::PushConstantRange const& pc, vk::ShaderStageFlagBits shaderStage)
    {
        pcRanges[shaderStage].emplace_back(pc);
    }

    void SetImmutableSamples(u8 dSetNum, uint8_t binding, vk::Sampler const* immutableSamplers)
    {
        dsLayouts[dSetNum][binding].pImmutableSamplers = immutableSamplers;
    }

    void SetDescriptorArraySize(u8 dSetNum, uint8_t binding, u32 size)
    {
        dsLayouts[dSetNum][binding].descriptorCount = size;
    }
};

template<typename Builder>
class Pipeline {
    ShaderCache& cache;
    std::vector<std::string_view> shaderNames;
    vk::UniquePipeline pipeline;

public:
    Builder pBuilder;
    PipelineLayout layout;

    [[nodiscard]] vk::Pipeline get() const
    {
        return pipeline.get();
    }

    [[nodiscard]] vk::PipelineLayout getLayout() const
    {
        return layout.pipeline.get();
    }

    Pipeline(vk::Device d, ShaderCache& cache, std::vector<std::string_view> _shaderNames)
        : cache(cache)
        , shaderNames(std::move(_shaderNames))
        , pBuilder(d)
    {
        PipelineLayoutBuilder lb;
        pBuilder.shaderStages.reserve(shaderNames.size());

        for (auto const& shaderName : shaderNames)
            lb.SetLayoutForShader(cache.LoadShader(shaderName));

        layout = lb.Build(d);
    }

    template<typename LayoutBuilderModifier>
    Pipeline(vk::Device d, ShaderCache& cache, std::vector<std::string_view> _shaderNames, LayoutBuilderModifier&& lbModify)
        : cache(cache)
        , shaderNames(std::move(_shaderNames))
        , pBuilder(d)
    {
        PipelineLayoutBuilder lb;
        pBuilder.shaderStages.reserve(shaderNames.size());

        for (auto const& shaderName : shaderNames)
            lb.SetLayoutForShader(cache.LoadShader(shaderName));

        std::forward<LayoutBuilderModifier>(lbModify)(lb);
        layout = lb.Build(d);
    }

    void build(vk::RenderPass renderPass = {}, u32 const subPassID = {})
    {
        pBuilder.shaderStages.clear();
        pBuilder.shaderStages.reserve(shaderNames.size());
        for (auto const& shaderName : shaderNames)
            pBuilder.shaderStages.emplace_back(cache.getShaderCreateInfo(shaderName));

        if constexpr (std::is_same_v<Builder, pipeline::BuilderGraphics>)
            pipeline = pBuilder.Build(layout.pipeline.get(), renderPass, subPassID, cache.getPipelineCache());
        if constexpr (std::is_same_v<Builder, pipeline::BuilderRayTracing>)
            pipeline = pBuilder.Build(layout.pipeline.get(), cache.getPipelineCache());
    }
};

using PipelineGraphics = Pipeline<pipeline::BuilderGraphics>;
using PipelineRayTracing = Pipeline<pipeline::BuilderRayTracing>;

}
