#pragma once

#include "core/ConfigFiles.h"

struct State {
    inline static constexpr bool FAST_EXIT { true };

    bool showImguiDemo = false;

    struct {
        u32 x = 0;
        u32 y = 0;
    } windowSize;

    ConfigFiles::Scene sceneToLoad {};
    bool sceneLoading = false;
    f32 statusBarFade { 1.0f };
    f32 progress { 0.f };

    u64 frameId = 0;
    f64 time { 0. };
    f64 deltaTime { 0. };

    bool shaderHotReload { true };
};
