#include "../include/vLime/Reflection.h"

#include <spirv_common.hpp>
#include <spirv_reflect.hpp>
#include <vLime/Util.h>

namespace lime {

Shader::LayoutReflection Shader::reflect(std::vector<uint32_t> spv, vk::ShaderStageFlagBits shaderStage)
{
    Shader::LayoutReflection result;

    spirv_cross::Compiler const comp { std::move(spv) };
    spirv_cross::ShaderResources const resources { comp.get_shader_resources() };

    vk::DescriptorSetLayoutBinding binding;
    binding.stageFlags = shaderStage;

    for (auto const& r : resources.uniform_buffers) {
        binding.descriptorType = vk::DescriptorType::eUniformBufferDynamic;
        binding.descriptorCount = 1;
        binding.binding = comp.get_decoration(r.id, spv::DecorationBinding);
        result.bindings.emplace_back(std::make_pair(comp.get_decoration(r.id, spv::DecorationDescriptorSet), binding));
    }

    for (auto const& r : resources.storage_buffers) {
        binding.descriptorType = vk::DescriptorType::eStorageBufferDynamic;
        binding.descriptorCount = 1;
        binding.binding = comp.get_decoration(r.id, spv::DecorationBinding);
        result.bindings.emplace_back(std::make_pair(comp.get_decoration(r.id, spv::DecorationDescriptorSet), binding));
    }

    for (auto const& r : resources.sampled_images) {
        binding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        binding.descriptorCount = 1;
        binding.binding = comp.get_decoration(r.id, spv::DecorationBinding);
        result.bindings.emplace_back(std::make_pair(comp.get_decoration(r.id, spv::DecorationDescriptorSet), binding));
    }

    for (auto const& r : resources.storage_images) {
        binding.descriptorType = vk::DescriptorType::eStorageImage;
        binding.descriptorCount = 1;
        binding.binding = comp.get_decoration(r.id, spv::DecorationBinding);
        result.bindings.emplace_back(std::make_pair(comp.get_decoration(r.id, spv::DecorationDescriptorSet), binding));
    }

    for (auto const& r : resources.acceleration_structures) {
        binding.descriptorType = vk::DescriptorType::eAccelerationStructureKHR;
        binding.descriptorCount = 1;
        binding.binding = comp.get_decoration(r.id, spv::DecorationBinding);
        result.bindings.emplace_back(std::make_pair(comp.get_decoration(r.id, spv::DecorationDescriptorSet), binding));
    }

    for (auto const& r : resources.push_constant_buffers) {
        vk::PushConstantRange pc;
        pc.stageFlags = shaderStage;

        auto& type = comp.get_type(r.base_type_id);
        for (u32 i = 0; i < type.member_types.size(); i++) {
            pc.size = static_cast<uint32_t>(comp.get_declared_struct_member_size(type, i));
            pc.offset = comp.type_struct_member_offset(type, i);
            result.pcRanges.emplace_back(pc);
        }
    }

    return result;
}

vk::ShaderStageFlagBits ShaderCache::stageFlagFromFileName(std::string_view name)
{
    if (name.find(".vert") != std::string_view::npos)
        return vk::ShaderStageFlagBits::eVertex;
    if (name.find(".tesc") != std::string_view::npos)
        return vk::ShaderStageFlagBits::eTessellationControl;
    if (name.find(".tese") != std::string_view::npos)
        return vk::ShaderStageFlagBits::eTessellationEvaluation;
    if (name.find(".geom") != std::string_view::npos)
        return vk::ShaderStageFlagBits::eGeometry;
    if (name.find(".frag") != std::string_view::npos)
        return vk::ShaderStageFlagBits::eFragment;
    if (name.find(".comp") != std::string_view::npos)
        return vk::ShaderStageFlagBits::eCompute;
    if (name.find(".rgen") != std::string_view::npos)
        return vk::ShaderStageFlagBits::eRaygenKHR;
    if (name.find(".rint") != std::string_view::npos)
        return vk::ShaderStageFlagBits::eIntersectionKHR;
    if (name.find(".rahit") != std::string_view::npos)
        return vk::ShaderStageFlagBits::eAnyHitKHR;
    if (name.find(".rchit") != std::string_view::npos)
        return vk::ShaderStageFlagBits::eClosestHitKHR;
    if (name.find(".rmiss") != std::string_view::npos)
        return vk::ShaderStageFlagBits::eMissKHR;
    if (name.find(".rcall") != std::string_view::npos)
        return vk::ShaderStageFlagBits::eCallableKHR;
    if (name.find(".mesh") != std::string_view::npos)
        return vk::ShaderStageFlagBits::eMeshNV;
    if (name.find(".task") != std::string_view::npos)
        return vk::ShaderStageFlagBits::eTaskNV;

    return vk::ShaderStageFlagBits::eAll;
}

void PipelineLayoutBuilder::SetLayoutForShader(Shader const& shader)
{
    for (auto const& b : shader.layoutReflection.bindings)
        AddDescriptorSetBinding(b.first, b.second);

    for (auto const& pc : shader.layoutReflection.pcRanges)
        AddPushConstantRange(pc, shader.stage);
}

void PipelineLayoutBuilder::AddDescriptorSetBinding(u32 dSetNum, vk::DescriptorSetLayoutBinding const& dsBinding)
{
    if (dSetNum >= dsLayouts.size())
        dsLayouts.resize(dSetNum + 1);

    auto& dsLayoutBindings { dsLayouts[dSetNum] };
    if (dsBinding.binding >= dsLayoutBindings.size())
        dsLayoutBindings.resize(dsBinding.binding + 1);

    auto& dsLayoutBinding { dsLayoutBindings[dsBinding.binding] };
    if (dsLayoutBinding.descriptorCount == 0)
        dsLayoutBinding = dsBinding;
    else
        dsLayoutBinding.stageFlags |= dsBinding.stageFlags;
}

PipelineLayout PipelineLayoutBuilder::Build(vk::Device d) const
{
    PipelineLayout result;

    result.dSet.reserve(dsLayouts.size());
    for (auto const& dsLayout : dsLayouts) {
        vk::DescriptorSetLayoutCreateInfo cInfo;
        cInfo.bindingCount = csize<u32>(dsLayout);
        cInfo.pBindings = dsLayout.data();

        result.dSet.emplace_back(d.createDescriptorSetLayoutUnique(cInfo).value);
    }

    std::vector<vk::DescriptorSetLayout> pSetLayouts;
    pSetLayouts.reserve(result.dSet.size());
    for (auto const& l : result.dSet)
        pSetLayouts.emplace_back(l.get());

    vk::PipelineLayoutCreateInfo cInfo;
    cInfo.setLayoutCount = csize<u32>(pSetLayouts);
    cInfo.pSetLayouts = pSetLayouts.data();

    std::vector<vk::PushConstantRange> pushConstants;
    if (!pcRanges.empty()) {
        for (auto const& [stage, stagePcs] : pcRanges) {
            u32 beginAddress = std::numeric_limits<u32>::max();
            u32 endAddress = std::numeric_limits<u32>::min();
            for (auto const& [stageFlags, offset, size] : stagePcs) {
                beginAddress = std::min(beginAddress, offset);
                endAddress = std::max(endAddress, offset + size);
            }
            pushConstants.emplace_back(stage, beginAddress, endAddress - beginAddress);
        }
        cInfo.pushConstantRangeCount = csize<u32>(pushConstants);
        cInfo.pPushConstantRanges = pushConstants.data();
    }

    result.pipeline = d.createPipelineLayoutUnique(cInfo).value;
    return result;
}

}
