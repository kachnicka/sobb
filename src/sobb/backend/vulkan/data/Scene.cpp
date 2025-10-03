#include "Scene.h"

#include "../../../scene/Scene.h"
#include <glm/gtc/type_ptr.hpp>

namespace backend::vulkan::data {

Scene::Scene(DeviceData* data, ::HostScene const& sceneOnHost)
    : data(data)
{
    for (auto const& geometry : sceneOnHost.geometries) {
        backend::input::Geometry g {
            .vertexCount = static_cast<uint32_t>(geometry.vertices.size()),
            .indexCount = static_cast<uint32_t>(geometry.indices.size()),
            .vertexData = geometry.vertices.data(),
            .indexData = geometry.indices.data(),
            .uvData = nullptr,
            .normalData = geometry.normals.data(),
        };

        auto gId { data->geometries.add(g) };
        geometries.emplace_back(gId);
        totalTriangleCount += g.indexCount / 3;
    }

    aabb = sceneOnHost.aabb;
    data->SetSceneDescription_TMP();
}

void Scene::UpdateTransformationMatrices(::HostScene const& sceneOnHost)
{
    static_cast<void>(sceneOnHost);
}

}
