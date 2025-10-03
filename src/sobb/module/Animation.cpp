#include "Animation.h"

#include "../Application.h"

namespace module {

Animation::Animation(Application& app)
    : app(app)
    , backend(app.backend)
{
}

void Animation::Run()
{
    static u32 frameInLevel { 0 };
    static u32 framesPreLevel { 0 };
    static u32 level { 0 };
    static Camera camera;

    if (!animation.running)
        return;

    if (animation.frame == 0) {
        app.backend.state.samplesPerPixel = animation.spp;
        frameInLevel = 0;
        framesPreLevel = animation.framesPerLevel;

        auto cfg { app.backend.GetConfig() };
        cfg.tracer.bvDepth = animation.minDepth;
        level = animation.minDepth;
        app.backend.SetPipelineConfiguration(cfg);
    }

    if (app.backend.state.samplesToComputeThisFrame > 0)
        return;

    if (animation.saveImages) {
        auto const path { app.directory.res / "animation" / std::format("{}_{:02d}_{:02d}", animation.name_TMP, level, frameInLevel) };
        std::filesystem::create_directories(path.parent_path());
        app.saveRenderWindowAsImage(path);
    }

    auto cfg { app.backend.GetConfig() };

    app.cameraManager.camera.RotationStart();
    app.cameraManager.camera.AnimationRotation_TMP(animation.speed);
    app.cameraManager.camera.PanStart();
    app.cameraManager.camera.AnimationPan_TMP(animation.panX, animation.panY);
    app.cameraManager.camera.Zoom(animation.zoom);
    app.cameraManager.camera.SetIdleState();

    // store camera state for transitions
    if (frameInLevel == framesPreLevel - 2 * animation.levelTransitionFrames)
        camera = app.cameraManager.camera;

    if (frameInLevel + 1 == framesPreLevel) {
        cfg.tracer.bvDepth++;
        level = cfg.tracer.bvDepth;

        app.backend.SetPipelineConfiguration(cfg);

        if (animation.levelTransitionFrames > 0)
            app.cameraManager.camera = camera;

        frameInLevel = 0;
        framesPreLevel = animation.framesPerLevel + animation.levelTransitionFrames;
    } else
        frameInLevel++;

    animation.frame++;

    if (static_cast<i32>(cfg.tracer.bvDepth) == animation.maxDepth) {
        animation.frame = 0;
        animation.running = false;
    }
}

}
