#include <vLime/Capability.h>

namespace lime::device {

Features::Features(vk::PhysicalDevice const pd)
{
    auto& features { GetFeaturesChainFull() };
    pd.getFeatures2(&features);
}

vk::PhysicalDeviceFeatures2& Features::GetFeaturesChain()
{
    void** pNext = &features2.pNext;

    if (vulkan11FeaturesSet) {
        *pNext = &vulkan11Features;
        pNext = &vulkan11Features.pNext;
    }
    if (vulkan12FeaturesSet) {
        *pNext = &vulkan12Features;
        pNext = &vulkan12Features.pNext;
    }
    if (vulkan13FeaturesSet) {
        *pNext = &vulkan13Features;
        pNext = &vulkan13Features.pNext;
    }
    if (asFeaturesSet) {
        *pNext = &asFeatures;
        pNext = &asFeatures.pNext;
    }
    if (rtFeaturesSet) {
        *pNext = &rtFeatures;
        pNext = &rtFeatures.pNext;
    }
    if (atomicFloatFeaturesSet) {
        *pNext = &atomicFloatFeatures;
        pNext = &atomicFloatFeatures.pNext;
    }
    if (clockFeaturesSet) {
        *pNext = &clockFeatures;
        pNext = &clockFeatures.pNext;
    }

    return features2;
}

vk::PhysicalDeviceFeatures2& Features::GetFeaturesChainFull()
{
    features2.pNext = &vulkan11Features;
    vulkan11Features.pNext = &vulkan12Features;
    vulkan12Features.pNext = &vulkan13Features;
    vulkan13Features.pNext = &asFeatures;
    asFeatures.pNext = &rtFeatures;
    rtFeatures.pNext = &atomicFloatFeatures;
    atomicFloatFeatures.pNext = &clockFeatures;

    return features2;
}

}