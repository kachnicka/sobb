#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <unordered_map>

#include "../backend/Config.h"

class ConfigFiles {
public:
    struct Scene {
        std::string name;
        std::string path;
        std::string camera;
        std::string bin;
    };
    std::vector<Scene> scenes;
    std::vector<backend::config::BVHPipeline> bvhPipelines;
    std::vector<std::string> benchmarkScenes;
    std::vector<std::string> benchmarkPipelines;

    ConfigFiles(std::filesystem::path const& res, std::filesystem::path const& pScenes, std::filesystem::path const& pPipelines);
    void Read(std::filesystem::path const& res, std::filesystem::path const& pScenes, std::filesystem::path const& pPipelines);
    Scene GetScene(std::string_view = {});
    std::vector<ConfigFiles::Scene> const& GetScenes() const;

private:
    std::string startupScene;
    void parseScenes(std::filesystem::path const& res, std::filesystem::path const& pScenes);
    void parsePipelines(std::filesystem::path const& pPipelines);
};
