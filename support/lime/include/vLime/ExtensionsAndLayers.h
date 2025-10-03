#pragma once

#include <vLime/Capability.h>
#include <vLime/Vulkan.h>

namespace lime::extension {
std::vector<char const*> CheckExtensionsInstance(Capabilities& capabilities);
std::vector<char const*> CheckExtensionsDevice(Capabilities& capabilities, vk::PhysicalDevice pd);
}
namespace lime::validation {
void RemoveUnavailableLayers(std::vector<char const*>& layersRequested);
std::vector<char const*> GetDefaultLayers(bool const haveDebugUtils);
}
namespace lime::device {
Features CheckAndSetDeviceFeatures(Capabilities& capabilities, vk::PhysicalDevice pd, bool silent = false);
}
