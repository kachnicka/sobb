#include "ShaderManager.h"

#include <berries/lib_helper/spdlog.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <ranges>
#include <shared_mutex>
#include <string_view>

namespace module {
namespace fs = std::filesystem;

struct ShaderManager::Impl {
private:
    std::filesystem::path shaders;
    struct TrackedFile {
        std::string name;
        fs::path path;
        fs::file_time_type timestamp;
        std::function<void(std::string_view, std::vector<u32>)> onChangeCallback;
    };
    std::vector<TrackedFile> trackedFiles;
    std::shared_mutex trackedFilesMutex;

    void fastRemove(fs::path const& path)
    {
        if (trackedFiles.empty())
            return;

        std::unique_lock lock { trackedFilesMutex };

        auto it = std::ranges::find_if(trackedFiles, [&path](auto const& file) {
            return path == file.path;
        });

        if (it != trackedFiles.end()) {
            std::iter_swap(it, trackedFiles.end() - 1);
            trackedFiles.pop_back();
        }
    }

    [[nodiscard]] bool validate(fs::path const& path)
    {
        auto const status = fs::status(path);
        if (status.type() == fs::file_type::not_found || !fs::is_regular_file(status)) {
            berry::log::error("File '{}' does not exist.", path.generic_string());
            return false;
        }
        if (status.type() == fs::file_type::unknown) {
            berry::log::error("File '{}' cannot be accessed.", path.generic_string());
            return false;
        }
        return true;
    }

    [[nodiscard]] std::vector<u32> readSpv(std::filesystem::path const& path)
    {
        std::ifstream file { path, std::ios::ate | std::ios::binary };

        if (!file.is_open()) {
            berry::log::error(std::format("Failed to open file '{}'!", path.generic_string()));
            abort();
        }

        size_t const fileSizeU8 = file.tellg();
        size_t const fileSizeU32 = file.tellg() >> 2;
        std::vector<u32> buffer(fileSizeU32);

        file.seekg(0);
        file.read(reinterpret_cast<char*>(buffer.data()), fileSizeU8);
        file.close();

        return buffer;
    }

public:
    Impl(std::filesystem::path const& pRes)
        : shaders(pRes / "shaders")
    {
    }

    void Load(std::string_view name, std::vector<u32>& data, std::function<void(std::string_view, std::vector<u32>)> onChangeCallback)
    {
        fs::path path { name };
        if (path.is_relative())
            path = shaders / path;

        if (!validate(path))
            return;

        data = readSpv(path);

        std::unique_lock lock { trackedFilesMutex };
        trackedFiles.push_back({
            .name = std::string(name),
            .path = path,
            .timestamp = fs::last_write_time(path),
            .onChangeCallback = std::move(onChangeCallback),
        });
        berry::log::debug("Now tracking '{}'.", path.generic_string());
    }

    void Unload(fs::path const& path)
    {
        fastRemove(path);
        berry::log::debug("Stopped tracking '{}'.", path.generic_string());
    }

    void CheckForHotReload()
    {
        std::shared_lock lock { trackedFilesMutex };
        for (auto& f : trackedFiles) {
            if (!validate(f.path))
                continue;

            if (auto const tsNow = fs::last_write_time(f.path); tsNow > f.timestamp) {
                berry::log::debug("File '{}' changed at {}.", f.path.generic_string(), std::format("{}", std::chrono::floor<std::chrono::seconds>(tsNow)));

                if (f.onChangeCallback)
                    f.onChangeCallback(f.name, readSpv(f.path));
                f.timestamp = tsNow;
            }
        }
    }
};

ShaderManager::ShaderManager(std::filesystem::path const& pRes)
    : impl(std::make_unique<Impl>(pRes))
{
}
ShaderManager::~ShaderManager() = default;

void ShaderManager::Load(std::string_view name, std::vector<u32>& data, std::function<void(std::string_view, std::vector<u32>)> onChangeCallback) { impl->Load(name, data, onChangeCallback); }
void ShaderManager::Unload(std::string_view name) { impl->Unload(name); }
void ShaderManager::CheckForHotReload() { impl->CheckForHotReload(); }

}
