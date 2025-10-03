#pragma once

#include "../ApplicationState.h"
#include "../backend/vulkan/Vulkan.h"

class Application;
namespace module {

class Animation {
public:
    explicit Animation(Application& app);
    void ProcessGUI(State& _state);

    void Run();

private:
    Application& app;
    backend::vulkan::Vulkan& backend;

    struct {
        u32 frame { 0 };
        bool running { false };

        i32 spp { 1 };
        i32 minDepth { 0 };
        i32 maxDepth { 20 };
        f32 speed { 1.75f };
        f32 zoom { .0008f };
        f32 panX { .025f };
        f32 panY { -.03f };

        i32 framesPerLevel { 18 };
        i32 levelTransitionFrames { 3 };

        std::string name_TMP { "animation" };
        bool saveImages { true };
    } animation;

    enum class AnimationState {
        eIdle,
    } state { AnimationState::eIdle };
};

}
