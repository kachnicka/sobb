#pragma once

#include "../ApplicationState.h"
#include <functional>

struct State;
namespace module {

class ShaderManager {
    struct Impl;
    std::unique_ptr<Impl> impl;

public:
    ShaderManager(std::filesystem::path const& pathResources);
    ~ShaderManager();

    void Load(std::string_view name, std::vector<u32>& data, std::function<void(std::string_view, std::vector<u32>)> onChangeCallback);
    void Unload(std::string_view name);
    void CheckForHotReload();
};

}
