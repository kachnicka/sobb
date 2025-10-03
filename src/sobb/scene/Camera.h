#pragma once

#if defined(_MSC_VER)
#    pragma warning(push)
#    pragma warning(disable : 4201)
#endif

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#if defined(_MSC_VER)
#    pragma warning(pop)
#endif

#include <algorithm>
#include <array>
#include <iostream>

#include "../backend/data/Input.h"
#include "AABB.h"
#include <berries/lib_helper/spdlog.h>

class Camera {
    static float constexpr RADIUS { 1.1f };
    static float constexpr SQRT_2 { 1.414213562373095f };
    static float constexpr RAD_PER_PIXEL { glm::pi<float>() / 400.f };
    static float constexpr NAVIGATION_EPSILON { 1e-3f };

public:
    glm::vec3 pivot { 0.f, 0.f, 25.f };
    glm::vec3 up { 0.f, 0.f, 1.f };
    // position relative to pivot point for the orbiting camera
    glm::vec3 position { 0.f, -3500.f, 0.f };
    glm::quat orientation { glm::identity<glm::quat>() };

    glm::vec3 pivotCache { 0.f, 0.f, 0.f };
    glm::quat zOrientation { glm::identity<glm::quat>() };
    glm::quat xOrientation { glm::identity<glm::quat>() };
    glm::quat zOrientationCache { glm::identity<glm::quat>() };
    glm::quat xOrientationCache { glm::identity<glm::quat>() };

    glm::mat4 viewMat { glm::identity<glm::mat4>() };
    // OpenGL NDC with Y axis pointing up
    glm::mat4 projectionOpenGL { glm::identity<glm::mat4>() };
    // Vulkan NDC with Y axis pointing down
    glm::mat4 projectionVulkan { glm::identity<glm::mat4>() };

    glm::u32vec2 screenResolution { 1, 1 };
    float yFov { glm::pi<float>() * .33f };
    float zNear { 1.3f };
    float zFar { 33333.f };

    float panStep { 1.f };
    float panFactor { 1.f };

    float zoomStep { 1.f };
    float zoomFactor { 1.f };

    void updateProjection()
    {
        float aspect { static_cast<float>(screenResolution.x) / static_cast<float>(screenResolution.y) };
        projectionOpenGL = glm::perspective(yFov, aspect, zNear, zFar);
        projectionVulkan = projectionOpenGL;
        projectionVulkan[1][1] *= -1.f;
        changed = true;
    }

    bool changed { true };

    enum class NavigationState : uint8_t {
        eIdle,
        eRotation,
        ePanning,
    } navigationState { NavigationState::eIdle };

    double xPosCurrent { 0. };
    double yPosCurrent { 0. };
    double xPosCache { 0. };
    double yPosCache { 0. };

public:
    [[maybe_unused]] bool UpdateCamera()
    {
        ProcessNavigation();

        auto const p { glm::vec3(mat4_cast(orientation) * glm::vec4(position, 1.f)) + pivot };
        auto const u { glm::vec3(mat4_cast(orientation) * glm::vec4(up, 0.f)) };
        viewMat = glm::lookAt(p, pivot, u);

        auto result { changed };
        changed = false;
        return result;
    }

    enum class AngleUnit {
        eRad,
        eDeg,
    };

    [[maybe_unused]] void SetFovY(float angle, AngleUnit unit = AngleUnit::eRad)
    {
        if (unit == AngleUnit::eRad)
            yFov = angle;
        else
            yFov = glm::radians(angle);
        updateProjection();
    }

    void SetNearFar(float n = .1f, float f = 100.f)
    {
        zNear = n;
        zFar = f;
        updateProjection();
    }

    bool ScreenResize(uint32_t x, uint32_t y)
    {
        if (x == screenResolution.x && y == screenResolution.y)
            return false;

        static uint32_t constexpr RESOLUTION_MIN { 16 };
        static uint32_t constexpr RESOLUTION_MAX { 8192 };
        screenResolution.x = std::clamp(x, RESOLUTION_MIN, RESOLUTION_MAX);
        screenResolution.y = std::clamp(y, RESOLUTION_MIN, RESOLUTION_MAX);
        updateProjection();
        return true;
    }

    void Zoom(float units)
    {
        position.y += units * zoomStep * zoomFactor;
        position.y = std::min(-NAVIGATION_EPSILON, position.y);
        changed = true;
    }

    void RotationStart()
    {
        navigationState = NavigationState::eRotation;
        xPosCache = xPosCurrent;
        yPosCache = yPosCurrent;

        zOrientationCache = zOrientation;
        xOrientationCache = xOrientation;
    }

    void PanStart()
    {
        navigationState = NavigationState::ePanning;
        xPosCache = xPosCurrent;
        yPosCache = yPosCurrent;

        pivotCache = pivot;
    }

    void SetIdleState()
    {
        navigationState = NavigationState::eIdle;
    }

    void ProcessNavigation()
    {
        switch (navigationState) {
        case NavigationState::eIdle:
            return;
        case NavigationState::eRotation:
            Rotate();
            return;
        case NavigationState::ePanning:
            Pan();
            return;
        }
    }

    // rotates around global Z-up axis and local Y axis
    void Rotate()
    {
        float dx { static_cast<float>(xPosCache - xPosCurrent) };
        float dy { static_cast<float>(yPosCache - yPosCurrent) };

        if ((std::abs(dx) < NAVIGATION_EPSILON) && (std::abs(dy) < NAVIGATION_EPSILON))
            return;

        glm::quat change_1 { glm::vec3 { 0.f, 0.f, dx } * RAD_PER_PIXEL };
        glm::quat change_2 { glm::vec3 { dy, 0.f, 0.f } * RAD_PER_PIXEL };

        zOrientation = zOrientationCache * change_1;
        xOrientation = xOrientationCache * change_2;
        orientation = zOrientation * xOrientation;

        changed = true;
    }

    void AnimationRotation_TMP(float change = .2f)
    {
        glm::quat change_1 { glm::vec3 { 0.f, 0.f, change } * RAD_PER_PIXEL };

        zOrientation = zOrientationCache * change_1;
        orientation = zOrientation * xOrientation;

        changed = true;
    }

    void Pan()
    {
        auto const dx = static_cast<f32>(xPosCache - xPosCurrent);
        auto const dy = static_cast<f32>(yPosCache - yPosCurrent);

        // pivot cancels out in view vector calculation, as position is in relative coordinates
        auto const view { glm::normalize(glm::vec3(mat4_cast(orientation) * glm::vec4(position, 1.f))) };
        auto const actualUp { glm::vec3(mat4_cast(orientation) * glm::vec4(up, 0.f)) };
        auto const right { glm::cross(view, actualUp) };

        pivot = pivotCache - (right * dx + actualUp * dy) * panStep * panFactor;

        changed = true;
    }

    void AnimationPan_TMP(float dx, float dy)
    {
        // pivot cancels out in view vector calculation, as position is in relative coordinates
        auto const view { glm::normalize(glm::vec3(mat4_cast(orientation) * glm::vec4(position, 1.f))) };
        auto const actualUp { glm::vec3(mat4_cast(orientation) * glm::vec4(up, 0.f)) };
        auto const right { glm::cross(view, actualUp) };

        pivot = pivotCache - (right * dx + actualUp * dy) * panFactor;

        changed = true;
    }

    void Fit(scene::AABB const& aabb)
    {
        resetNavigationCache();

        auto const radius { aabb.Radius() };
        pivot = aabb.Centroid();
        position = glm::vec3 { 0.f, -(radius / glm::tan(yFov * .5f)), 0.f };
        orientation = glm::identity<glm::quat>();

        panStep = radius * .005f;
        zoomStep = radius * .1f;

        changed = true;
    }

    [[nodiscard]] backend::input::Camera GetMatrices() const
    {
        backend::input::Camera c;
        memcpy(c.view.data(), glm::value_ptr(viewMat), sizeof(c.view));
        memcpy(c.viewInv.data(), glm::value_ptr(glm::inverse(viewMat)), sizeof(c.viewInv));
        memcpy(c.projection.data(), glm::value_ptr(projectionVulkan), sizeof(c.projection));
        memcpy(c.projectionInv.data(), glm::value_ptr(glm::inverse(projectionVulkan)), sizeof(c.projectionInv));
        return c;
    }

    friend std::ostream& operator<<(std::ostream& os, Camera const& c);
    friend std::istream& operator>>(std::istream& is, Camera& c);

private:
    void resetNavigationCache()
    {
        pivotCache = { 0.f, 0.f, 0.f };
        zOrientation = glm::identity<glm::quat>();
        xOrientation = glm::identity<glm::quat>();
        zOrientationCache = glm::identity<glm::quat>();
        xOrientationCache = glm::identity<glm::quat>();
    }
};

inline static std::ostream& serialize(std::ostream& os, glm::vec3 const& v)
{
    os << v.x << " " << v.y << " " << v.z << " ";
    return os;
}
inline static std::ostream& serialize(std::ostream& os, glm::quat const& q)
{
    os << q.x << " " << q.y << " " << q.z << " " << q.w << " ";
    return os;
}
inline static std::istream& deserialize(std::istream& is, glm::vec3& v)
{
    is >> v.x >> v.y >> v.z;
    return is;
}
inline static std::istream& deserialize(std::istream& is, glm::quat& q)
{
    is >> q.x >> q.y >> q.z >> q.w;
    return is;
}

inline std::ostream& operator<<(std::ostream& os, Camera const& c)
{
    serialize(os, c.pivot);
    serialize(os, c.up);
    serialize(os, c.position);
    serialize(os, c.orientation);

    serialize(os, c.zOrientation);
    serialize(os, c.xOrientation);
    serialize(os, c.zOrientationCache);
    serialize(os, c.xOrientationCache);

    os << c.yFov << " " << c.zNear << " " << c.zFar << " ";
    os << c.panStep << " " << c.panFactor << " " << c.zoomStep << " " << c.zoomFactor << "\n";

    return os;
}

inline std::istream& operator>>(std::istream& is, Camera& c)
{
    deserialize(is, c.pivot);
    deserialize(is, c.up);
    deserialize(is, c.position);
    deserialize(is, c.orientation);

    deserialize(is, c.zOrientation);
    deserialize(is, c.xOrientation);
    deserialize(is, c.zOrientationCache);
    deserialize(is, c.xOrientationCache);

    is >> c.yFov >> c.zNear >> c.zFar;
    is >> c.panStep >> c.panFactor >> c.zoomStep >> c.zoomFactor;
    return is;
}
