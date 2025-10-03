#pragma once

#include <vLime/vLime.h>

namespace backend::vulkan::vertex_layout {

struct ImGUI {
    static inline std::array<vk::VertexInputBindingDescription, 1> GetBindingDescriptions()
    {
        std::array<vk::VertexInputBindingDescription, 1> bindingDescriptions;
        bindingDescriptions[0].binding = 0;
        bindingDescriptions[0].stride = 5 * sizeof(float);
        bindingDescriptions[0].inputRate = vk::VertexInputRate::eVertex;

        return bindingDescriptions;
    }

    static inline std::array<vk::VertexInputAttributeDescription, 3> GetAttributeDescriptions()
    {
        std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions;

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = vk::Format::eR32G32Sfloat;
        attributeDescriptions[0].offset = 0 * sizeof(float);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = vk::Format::eR32G32Sfloat;
        attributeDescriptions[1].offset = 2 * sizeof(float);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = vk::Format::eR8G8B8A8Unorm;
        attributeDescriptions[2].offset = 4 * sizeof(float);

        return attributeDescriptions;
    }
};

}
