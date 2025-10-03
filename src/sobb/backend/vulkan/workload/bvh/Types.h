#pragma once

#include "../../../Config.h"
#include <vLime/Memory.h>
#include <vLime/types.h>
#include <vLime/vLime.h>

namespace backend::vulkan::bvh {

struct Bvh {
    vk::DeviceAddress bvh { 0 };
    vk::DeviceAddress triangles { 0 };
    vk::DeviceAddress triangleIDs { 0 };
    vk::DeviceAddress bvhAux { 0 };

    u32 nodeCountLeaf { 0 };
    u32 nodeCountTotal { 0 };

    config::BV bv { config::BV::eNone };
    config::NodeLayout layout { config::NodeLayout::eDefault };

    bool isValid() const
    {
        return bvh != 0;
    }

    bool operator==(Bvh const& rhs) const = default;
};

struct BvhStats {
    f32 saTraverse { 0.f };
    f32 saIntersect { 0.f };
    f32 costTraverse { 0.f };
    f32 costIntersect { 0.f };
    u32 leafSizeSum { 0 };
    u32 leafSizeMin { 0xFFFFFFFF };
    u32 leafSizeMax { 0 };
};

struct TraceRuntime {
    struct {
        u32 computed { 0 };
        u32 toCompute { 0 };
    } samples;
    u32 x { 0 };
    u32 y { 0 };

    u32 ptDepth { 7 };

    vk::DeviceAddress geometryDescriptorAddress { 0 };

    vk::ImageView targetImageView;
    lime::Buffer::Detail camera;
};

}
