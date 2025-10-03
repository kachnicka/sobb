#pragma once

#include "../../../scene/AABB.h"
#include "DeviceData.h"
#include "handler/Geometry.h"
#include <atomic>
#include <berries/util/UidUtil.h>
#include <vector>

struct HostScene;

namespace backend::vulkan::data {

struct Scene {
    struct {
        std::atomic<u64> loadedOnFrame { std::numeric_limits<u64>::max() };
        bool everything { false };
        bool transformationMatrices { false };

        void set(u64 frameId)
        {
            if (loadedOnFrame <= frameId) {
                everything = true;
                loadedOnFrame = std::numeric_limits<u64>::max();
            }
        }
        void reset()
        {
            everything = false;
            transformationMatrices = false;
        }
    } changed;

    DeviceData* data { nullptr };

    scene::AABB aabb;
    u32 totalTriangleCount { 0 };
    std::vector<Geometry::ID> geometries;

    Scene() = default;
    Scene(DeviceData* data, ::HostScene const& sceneOnHost);
    void UpdateTransformationMatrices(::HostScene const& sceneOnHost);
};

}
