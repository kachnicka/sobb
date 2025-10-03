#pragma once

#include <vLime/vLime.h>

namespace lime {
class Image;
}
namespace lime::rg {
class Graph;
}

namespace backend::vulkan {
struct State;
namespace data {
struct Scene;
}

struct Renderer {
    virtual ~Renderer() = default;

    virtual void Update(State const&) = 0;
    virtual lime::rg::id::Resource ScheduleRgTasks(lime::rg::Graph& rg, data::Scene const& scene) = 0;
    virtual lime::Image const& GetBackbuffer() const = 0;
    virtual void Resize(u32 x, u32 y) = 0;
};

}
