#pragma once

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

using f32 = float;
#include <algorithm>

namespace scene {
struct AABB {
    glm::vec3 min { std::numeric_limits<f32>::max() };
    glm::vec3 max { std::numeric_limits<f32>::lowest() };

    [[nodiscard]] f32 Area() const
    {
        auto const d { max - min };
        return 2.f * (d.x * d.y + d.x * d.z + d.z * d.y);
    }

    [[nodiscard]] glm::vec3 Centroid() const
    {
        return (min + max) * .5f;
    }

    [[nodiscard]] f32 Radius() const
    {
        return glm::length(max - min) * .5f;
    }

    [[nodiscard]] AABB GetCubed() const
    {
        auto const diagonal { max - min };
        auto const maxEdge { std::max(std::max(diagonal.x, diagonal.y), diagonal.z) };
        return { .min = min, .max = min + maxEdge };
    }

    void Fit(glm::vec3 const& vertex)
    {
        min = glm::min(min, vertex);
        max = glm::max(max, vertex);
    }
};

}
