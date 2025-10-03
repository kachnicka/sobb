#pragma once

#include "spdlog.h"
#include <GLFW/glfw3.h>
#include <iostream>

#ifdef VK_USE_PLATFORM_WIN32_KHR
#    define GLFW_EXPOSE_NATIVE_WIN32
#    include <GLFW/glfw3native.h>
#    include <windows.h>
#endif

#ifdef VK_USE_PLATFORM_XLIB_KHR
#    define GLFW_EXPOSE_NATIVE_X11
#    include <GLFW/glfw3native.h>
#    include <X11/Xlib.h>
#endif

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
#    define GLFW_EXPOSE_NATIVE_WAYLAND
#    include <GLFW/glfw3native.h>
#    include <wayland-client.h>
#endif

namespace berry {

class Window {
    static void myGlfwErrorCallback(int error, char const* description)
    {
        std::cerr << "GLFW Error '" << error << "': " << description << std::endl;
    }
    static void myGlfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        static_cast<void>(scancode);
        static_cast<void>(mods);

        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

public:
    GLFWwindow* window = nullptr;
    int width { 0 };
    int height { 0 };

public:
    Window(int width, int height, char const* title, GLFWkeyfun keyCallback = myGlfwKeyCallback)
        : width(width)
        , height(height)
    {
        glfwSetErrorCallback(myGlfwErrorCallback);
        if (!glfwInit())
            exit(1);

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(width, height, title, nullptr, nullptr);

        if (!window) {
            glfwTerminate();
            exit(0);
        }

        glfwSetKeyCallback(window, keyCallback);
        berry::log::timer("Window initialized.", glfwGetTime());
    }

    ~Window()
    {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    [[nodiscard]] bool ShouldClose() const
    {
        return glfwWindowShouldClose(window);
    }

#ifdef VK_USE_PLATFORM_WIN32_KHR
    [[nodiscard]] static HINSTANCE GetWin32ModuleHandle()
    {
        return GetModuleHandle(nullptr);
    }
    [[nodiscard]] HWND GetWin32Window() const
    {
        return glfwGetWin32Window(window);
    }
#endif

#ifdef VK_USE_PLATFORM_XLIB_KHR
    [[nodiscard]] Display* GetX11Display() const
    {
        return glfwGetX11Display();
    }
    [[nodiscard]] ::Window GetX11Window() const
    {
        return glfwGetX11Window(window);
    }
#endif

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    [[nodiscard]] wl_display* GetWaylandDisplay() const
    {
        return glfwGetWaylandDisplay();
    }
    [[nodiscard]] wl_surface* GetWaylandSurface() const
    {
        return glfwGetWaylandWindow(window);
    }
#endif
};

}
