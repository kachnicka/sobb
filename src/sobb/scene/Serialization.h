#pragma once

#include "Scene.h"
#include <filesystem>

namespace scene {

void serialize(HostScene const& scene, std::filesystem::path const& dir);
HostScene deserialize(std::filesystem::path const& path);

}
