#pragma once

#include <vLime/Capability.h>

namespace backend::vulkan {

class BasicGraphics final : public lime::Capability {
public:
    [[nodiscard]] char const* getName() const override
    {
        return "Basic Graphics";
    }

    lime::device::Features::Availability checkAndSetDeviceFeatures(lime::device::Features const& available, lime::device::Features& enabled) const override
    {
        bool result = true;
        result &= (available.features2.features.samplerAnisotropy == vk::True);
        result &= (available.features2.features.sampleRateShading == vk::True);
        result &= (available.features2.features.fillModeNonSolid == vk::True);
        result &= (available.features2.features.wideLines == vk::True);
        // result &= (available.features2.features.shaderClipDistance == vk::True);
        // result &= (available.features2.features.dualSrcBlend == vk::True);
        result &= (available.vulkan12Features.runtimeDescriptorArray == vk::True);

        if (result) {
            enabled.features2.features.samplerAnisotropy = vk::True;
            enabled.features2.features.sampleRateShading = vk::True;
            enabled.features2.features.fillModeNonSolid = vk::True;
            enabled.features2.features.wideLines = vk::True;
            // enabled.features2.features.shaderClipDistance = vk::True;
            // enabled.features2.features.dualSrcBlend = vk::True;

            enabled.vulkan12FeaturesSet = true;
            enabled.vulkan12Features.runtimeDescriptorArray = vk::True;

            return lime::device::Features::Availability::eAvailable;
        }
        return lime::device::Features::Availability::eNotAvailable;
    }
};

class RayTracing_KHR final : public lime::Capability {
public:
    [[nodiscard]] char const* getName() const override
    {
        return "Ray Tracing KHR";
    }
    [[nodiscard]] std::vector<char const*> extensionsDevice() const override
    {
        return {
            vk::KHRAccelerationStructureExtensionName,
            vk::KHRRayTracingPipelineExtensionName,
            vk::KHRDeferredHostOperationsExtensionName,
            vk::KHRShaderClockExtensionName,
        };
    }
    lime::device::Features::Availability checkAndSetDeviceFeatures(lime::device::Features const& available, lime::device::Features& enabled) const override
    {
        bool result = true;
        result &= (available.features2.features.shaderStorageImageWriteWithoutFormat == vk::True);
        result &= (available.features2.features.shaderFloat64 == vk::True);
        result &= (available.features2.features.shaderInt64 == vk::True);

        result &= (available.vulkan12Features.bufferDeviceAddress == vk::True);
        result &= (available.vulkan12Features.shaderBufferInt64Atomics == vk::True);

        result &= (available.asFeatures.accelerationStructure == vk::True);
        result &= (available.asFeatures.descriptorBindingAccelerationStructureUpdateAfterBind == vk::True);
        result &= (available.rtFeatures.rayTracingPipeline == vk::True);

        if (result) {
            enabled.features2.features.shaderStorageImageWriteWithoutFormat = vk::True;
            enabled.features2.features.shaderFloat64 = vk::True;
            enabled.features2.features.shaderInt64 = vk::True;

            enabled.vulkan12FeaturesSet = true;
            enabled.vulkan12Features.bufferDeviceAddress = vk::True;
            enabled.vulkan12Features.shaderBufferInt64Atomics = vk::True;

            enabled.asFeaturesSet = true;
            enabled.asFeatures.accelerationStructure = vk::True;
            enabled.asFeatures.descriptorBindingAccelerationStructureUpdateAfterBind = vk::True;

            enabled.rtFeaturesSet = true;
            enabled.rtFeatures.rayTracingPipeline = vk::True;

            return lime::device::Features::Availability::eAvailable;
        }
        return lime::device::Features::Availability::eNotAvailable;
    }
};

class RayTracing_compute final : public lime::Capability {
public:
    [[nodiscard]] char const* getName() const override
    {
        return "Ray Tracing compute";
    }
    [[nodiscard]] std::vector<char const*> extensionsDevice() const override
    {
        return {
            vk::KHRShaderClockExtensionName,
            vk::EXTShaderAtomicFloatExtensionName,

            // vk::KHRShaderRelaxedExtendedInstructionExtensionName,
        };
    }
    lime::device::Features::Availability checkAndSetDeviceFeatures(lime::device::Features const& available, lime::device::Features& enabled) const override
    {
        bool result = true;
        result &= (available.features2.features.shaderInt64 == vk::True);

        result &= (available.vulkan11Features.variablePointers == vk::True);
        result &= (available.vulkan11Features.variablePointersStorageBuffer == vk::True);

        result &= (available.vulkan12Features.bufferDeviceAddress == vk::True);
        result &= (available.vulkan12Features.vulkanMemoryModel == vk::True);
        result &= (available.vulkan12Features.vulkanMemoryModelDeviceScope == vk::True);
        result &= (available.vulkan12Features.shaderSharedInt64Atomics == vk::True);

        result &= (available.vulkan13Features.maintenance4 == vk::True);
        result &= (available.vulkan13Features.subgroupSizeControl == vk::True);

        // TODO: AMD Adrenaline 25.8.1 with 9070XT reports missing this feature
        // result &= (available.atomicFloatFeatures.shaderSharedFloat32AtomicAdd == vk::True);

        result &= (available.clockFeatures.shaderDeviceClock == vk::True);
        result &= (available.clockFeatures.shaderSubgroupClock == vk::True);

        if (result) {
            enabled.features2.features.shaderInt64 = vk::True;

            enabled.vulkan11FeaturesSet = true;
            enabled.vulkan11Features.variablePointers = vk::True;
            enabled.vulkan11Features.variablePointersStorageBuffer = vk::True;

            enabled.vulkan12FeaturesSet = true;
            enabled.vulkan12Features.bufferDeviceAddress = vk::True;
            enabled.vulkan12Features.vulkanMemoryModel = vk::True;
            enabled.vulkan12Features.vulkanMemoryModelDeviceScope = vk::True;
            enabled.vulkan12Features.shaderSharedInt64Atomics = vk::True;

            enabled.vulkan13FeaturesSet = true;
            enabled.vulkan13Features.maintenance4 = vk::True;
            enabled.vulkan13Features.subgroupSizeControl = vk::True;

            // TODO: see above, for now allow app to launch with errors
            if (available.atomicFloatFeatures.shaderSharedFloat32AtomicAdd == vk::True) {
                enabled.atomicFloatFeaturesSet = true;
                enabled.atomicFloatFeatures.shaderSharedFloat32AtomicAdd = vk::True;
            }

            enabled.clockFeaturesSet = true;
            enabled.clockFeatures.shaderDeviceClock = vk::True;
            enabled.clockFeatures.shaderSubgroupClock = vk::True;

            return lime::device::Features::Availability::eAvailable;
        }
        return lime::device::Features::Availability::eNotAvailable;
    }
};

class OnScreenPresentation final : public lime::Capability {
public:
    [[nodiscard]] char const* getName() const override
    {
        return "On screen presentation";
    }
    [[nodiscard]] std::vector<char const*> extensionsInstance() const override
    {
        return {
            vk::KHRSurfaceExtensionName,
#ifdef VK_USE_PLATFORM_WIN32_KHR
            vk::KHRWin32SurfaceExtensionName,
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
            vk::KHRXlibSurfaceExtensionName,
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
            vk::KHRWaylandSurfaceExtensionName,
#endif
        };
    }
    [[nodiscard]] std::vector<char const*> extensionsDevice() const override
    {
        return {
            vk::KHRSwapchainExtensionName,
        };
    }
};

class ExecutableProperties final : public lime::Capability {
public:
    [[nodiscard]] char const* getName() const override
    {
        return "Executable properties";
    }
    [[nodiscard]] std::vector<char const*> extensionsDevice() const override
    {
        return {
            vk::KHRPipelineExecutablePropertiesExtensionName,
        };
    }
};
}
