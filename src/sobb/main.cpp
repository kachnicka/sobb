#include "Application.h"
#include <berries/lib_helper/spdlog.h>

int main(int argc, char* argv[])
{
    berry::logger::init();
    berry::log::timer("Application start", glfwGetTime());

    Application app { argc, argv };
    auto const exitCode { app.Run() };

    berry::log::timer("Application end", glfwGetTime());
    berry::logger::deinit();
    return exitCode;
}
