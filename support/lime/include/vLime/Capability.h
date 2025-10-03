#pragma once

#include <memory>
#include <vLime/DebugUtils.h>
#include <vLime/Vulkan.h>
#include <vector>

namespace lime {
namespace device {
class Features {
public:
    enum class Availability {
        eAvailable,
        eNotAvailable,
        eDontCare,
    };

    vk::PhysicalDeviceFeatures2 features2;
    vk::PhysicalDeviceVulkan11Features vulkan11Features;
    vk::PhysicalDeviceVulkan12Features vulkan12Features;
    vk::PhysicalDeviceVulkan13Features vulkan13Features;
    vk::PhysicalDeviceAccelerationStructureFeaturesKHR asFeatures;
    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rtFeatures;
    vk::PhysicalDeviceShaderAtomicFloatFeaturesEXT atomicFloatFeatures;
    vk::PhysicalDeviceShaderClockFeaturesKHR clockFeatures;

    bool vulkan11FeaturesSet { false };
    bool vulkan12FeaturesSet { false };
    bool vulkan13FeaturesSet { false };
    bool asFeaturesSet { false };
    bool rtFeaturesSet { false };
    bool atomicFloatFeaturesSet { false };
    bool clockFeaturesSet { false };

    Features() = default;
    explicit Features(vk::PhysicalDevice pd);
    vk::PhysicalDeviceFeatures2& GetFeaturesChain();
    vk::PhysicalDeviceFeatures2& GetFeaturesChainFull();
};
}

class Capability {
public:
    virtual ~Capability() = default;
    [[nodiscard]] virtual char const* getName() const = 0;
    [[nodiscard]] virtual std::vector<char const*> extensionsInstance() const
    {
        return {};
    }
    [[nodiscard]] virtual std::vector<char const*> extensionsDevice() const
    {
        return {};
    }
    virtual device::Features::Availability checkAndSetDeviceFeatures(device::Features const& available, device::Features& enabled) const
    {
        static_cast<void>(available);
        static_cast<void>(enabled);
        return device::Features::Availability::eDontCare;
    }
};

class DebugUtils final : public Capability {
public:
    [[nodiscard]] char const* getName() const override
    {
        return "Debug Utils";
    }
    [[nodiscard]] std::vector<char const*> extensionsInstance() const override
    {
        return { vk::EXTDebugUtilsExtensionName };
    }
};

class DebugUtils_EXT_DeviceAddress final : public Capability {
public:
    [[nodiscard]] char const* getName() const override
    {
        return "Debug Utils EXT: Device Address";
    }
    [[nodiscard]] std::vector<char const*> extensionsDevice() const override
    {
        return { vk::EXTDeviceAddressBindingReportExtensionName };
    }
};

class Capabilities {
    std::vector<std::unique_ptr<Capability>> capabilities;

public:
    Capabilities()
    {
        if (debug::ENABLE_DEBUG_UTILS) {
            add<DebugUtils>();
            add<DebugUtils_EXT_DeviceAddress>();
        }
    }

    template<typename T>
    void add()
    {
        capabilities.emplace_back(std::make_unique<T>());
    }

    void remove(std::string_view const& name)
    {
        if (auto const it { std::find_if(capabilities.begin(), capabilities.end(), [&name](auto const& v) { return v->getName() == name; }) }; it != capabilities.end()) {
            *it = std::move(capabilities.back());
            capabilities.pop_back();
        }
    }

    [[nodiscard]] bool isAvailable(std::string_view name) const
    {
        return std::find_if(capabilities.begin(), capabilities.end(), [name](auto const& v) { return v->getName() == name; }) != capabilities.end();
    }

    template<typename T>
    [[nodiscard]] bool isAvailable() const
    {
        return std::find_if(capabilities.begin(), capabilities.end(), [name = T().getName()](auto const& v) { return v->getName() == name; }) != capabilities.end();
    }

    [[nodiscard]] std::vector<std::unique_ptr<Capability>>::iterator begin()
    {
        return capabilities.begin();
    }

    [[nodiscard]] std::vector<std::unique_ptr<Capability>>::const_iterator cbegin() const
    {
        return capabilities.cbegin();
    }

    [[nodiscard]] std::vector<std::unique_ptr<Capability>>::iterator end()
    {
        return capabilities.end();
    }

    [[nodiscard]] std::vector<std::unique_ptr<Capability>>::const_iterator cend() const
    {
        return capabilities.cend();
    }
};

}
