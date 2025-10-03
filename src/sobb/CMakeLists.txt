file(GLOB_RECURSE sobb_files CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(sobb ${sobb_files})
target_compile_features(sobb PUBLIC cxx_std_23)

get_target_property(EXECUTABLE_NAME sobb NAME)
set_target_properties(
    sobb
        PROPERTIES
            OUTPUT_NAME_RELEASE ${EXECUTABLE_NAME}
            OUTPUT_NAME_DEBUG ${EXECUTABLE_NAME}_d
)

if (WIN32)
    target_compile_options(sobb PRIVATE /W4 /bigobj)
    set_target_properties(
        sobb
            PROPERTIES
                MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    )
else()
    target_compile_options(sobb PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_include_directories(
    sobb
        PRIVATE
            "${CMAKE_HOME_DIRECTORY}/data/shaders/"
)

target_link_libraries(
    sobb
        PRIVATE
            Taskflow
            glfw imgui assimp glm
            spdlog::spdlog
            tomlplusplus::tomlplusplus
            berries::berries
            vLime::vLime
            vk-radix-sort
)

target_compile_definitions(
    sobb
        PUBLIC
            GLM_FORCE_DEPTH_ZERO_TO_ONE
)

set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY TRUE CACHE BOOL "" FORCE)
set(CMAKE_INSTALL_PREFIX "${CMAKE_HOME_DIRECTORY}/install" CACHE PATH "..." FORCE)
file(GLOB spv_files CONFIGURE_DEPENDS "${CMAKE_HOME_DIRECTORY}/data/shaders/final/*.spv")

install(FILES ${spv_files} DESTINATION ./data/shaders/final)
install(DIRECTORY ${CMAKE_HOME_DIRECTORY}/data/scene DESTINATION ./data)
install(DIRECTORY ${CMAKE_HOME_DIRECTORY}/data/font DESTINATION ./data)
install(FILES ${CMAKE_HOME_DIRECTORY}/data/benchmark.toml ${CMAKE_HOME_DIRECTORY}/data/scene.toml DESTINATION data/)
install(FILES ${CMAKE_HOME_DIRECTORY}/data/imgui.ini DESTINATION .)
install(TARGETS sobb RUNTIME DESTINATION .)
