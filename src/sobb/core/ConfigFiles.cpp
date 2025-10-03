#include "ConfigFiles.h"

#include <berries/lib_helper/spdlog.h>
#include <cstdlib>

#define TOML_IMPLEMENTATION
#define TOML_HEADER_ONLY 0
#include <toml++/toml.hpp>

namespace fs = std::filesystem;

backend::config::BVHPipeline getPipeline(toml::table const& tPipeline, toml::table const& tShaders);
void getPipeline(toml::table const& tPipeline, toml::table const& tShaders, backend::config::BVHPipeline& pipeline);

ConfigFiles::ConfigFiles(fs::path const& res, fs::path const& pScenes, fs::path const& pPipelines)
{
    Read(res, pScenes, pPipelines);
}

void ConfigFiles::Read(fs::path const& res, fs::path const& pScenes, fs::path const& pPipelines)
{
    parseScenes(res, pScenes);
    parsePipelines(pPipelines);
}

void ConfigFiles::parseScenes(fs::path const& res, fs::path const& pScenes)
{
    toml::table scenes_toml;
    try {
        scenes_toml = toml::parse_file(pScenes.c_str());
    } catch (toml::parse_error const& err) {
        berry::log::error("Parsing of '{}' failed:", pScenes.generic_string());
        berry::log::error("  {}", err.description());
        exit(EXIT_FAILURE);
    }
    if (auto v { scenes_toml["startup_scene"].value<std::string>() })
        startupScene = v.value();

    auto pathPrefixScene { scenes_toml["path_prefix_scene"].value<std::string_view>().value() };
    auto pathPrefixCamera { scenes_toml["path_prefix_camera"].value<std::string_view>().value() };

    auto resScene { (res / pathPrefixScene).generic_string() };
    auto resCamera { (res / pathPrefixCamera).generic_string() };

    if (!std::filesystem::path(pathPrefixScene).is_absolute())
        pathPrefixScene = resScene;
    if (!std::filesystem::path(pathPrefixCamera).is_absolute())
        pathPrefixCamera = resCamera;

    auto* scenes_array { scenes_toml["scenes"].as_array() };
    scenes.reserve(scenes_array->size());

    for (auto& scene : *scenes_array) {
        auto& s { *scene.as_table() };
        if (auto name { s["name"].value<std::string_view>() }; name) {
            Scene newScene;
            newScene.name = std::string(name.value());
            if (auto file { s["file"].value<std::string_view>() }; file && !file->empty())
                newScene.path = std::string(pathPrefixScene).append(file.value());
            if (auto cam { s["cam"].value<std::string_view>() }; cam && !cam->empty())
                newScene.camera = std::string(pathPrefixCamera).append(cam.value());
            if (auto file { s["bin"].value<std::string_view>() }; file && !file->empty())
                newScene.bin = std::string(pathPrefixScene).append(file.value());
            scenes.push_back(newScene);
        }
    }
}

void ConfigFiles::parsePipelines(fs::path const& pPipelines)
{
    toml::table toml;
    try {
        toml = toml::parse_file(pPipelines.c_str());
    } catch (toml::parse_error const& err) {
        berry::log::error("Parsing of '{}' failed:", pPipelines.generic_string());
        berry::log::error("  {}", err.description());
        exit(EXIT_FAILURE);
    }

    if (auto const value { toml["benchmark_scenes"].as_array() }) {
        benchmarkScenes.reserve(value->size());
        for (auto& v : *value)
            benchmarkScenes.push_back(v.as_string()->get());
    }
    if (auto const value { toml["benchmark_pipelines"].as_array() }) {
        benchmarkPipelines.reserve(value->size());
        for (auto& v : *value)
            benchmarkPipelines.push_back(v.as_string()->get());
    }
    auto* shaders { toml["shader"].as_table() };

    auto const defaultPipeline { getPipeline(*toml["default_pipeline"].as_table(), *shaders) };
    auto* pipelines { toml["benchmark"].as_array() };
    if (!pipelines)
        return;

    bvhPipelines.reserve(pipelines->size());
    for (auto& p : *pipelines) {
        auto const name { p.as_table()->at("name").value<std::string_view>() };
        if (!name)
            continue;

        // set base values for the pipeline
        auto& newPipeline = bvhPipelines.emplace_back(defaultPipeline);
        auto const parentName { p.as_table()->at_path("parent").value<std::string_view>() };
        if (parentName)
            for (auto const& bvhPipeline : bvhPipelines)
                if (bvhPipeline.name == parentName) {
                    newPipeline = bvhPipeline;
                    break;
                }
        newPipeline.name = std::string(name.value());
        getPipeline(*p.as_table(), *shaders, newPipeline);
    }
}

backend::config::BVHPipeline getPipeline(toml::table const& tPipeline, toml::table const& tShaders)
{
    backend::config::BVHPipeline pipeline;
    getPipeline(tPipeline, tShaders, pipeline);
    return pipeline;
}

std::vector<ConfigFiles::Scene> const& ConfigFiles::GetScenes() const
{
    return scenes;
}

ConfigFiles::Scene ConfigFiles::GetScene(std::string_view sceneName)
{
    Scene result;

    if (sceneName.empty()) {
        if (startupScene.empty())
            return {};
        sceneName = startupScene;
    }

    for (auto& scene : scenes)
        if (scene.name == sceneName)
            return scene;
    return {};
}

backend::config::BV getBoundingVolume(std::string_view bv)
{
    if (bv == "aabb")
        return backend::config::BV::eAABB;
    if (bv == "dop14")
        return backend::config::BV::eDOP14;
    if (bv == "dop14split")
        return backend::config::BV::eDOP14split;
    if (bv == "obb")
        return backend::config::BV::eOBB;
    if (bv == "sobb_d")
        return backend::config::BV::eSOBB_d;
    if (bv == "sobb_d32")
        return backend::config::BV::eSOBB_d32;
    if (bv == "sobb_d48")
        return backend::config::BV::eSOBB_d48;
    if (bv == "sobb_d64")
        return backend::config::BV::eSOBB_d64;
    if (bv == "sobb_i32")
        return backend::config::BV::eSOBB_i32;
    if (bv == "sobb_i48")
        return backend::config::BV::eSOBB_i48;
    if (bv == "sobb_i64")
        return backend::config::BV::eSOBB_i64;
    return backend::config::BV::eNone;
}

backend::config::NodeLayout getNodeLayout(std::string_view layout)
{
    if (layout == "bvh2")
        return backend::config::NodeLayout::eBVH2;
    return backend::config::NodeLayout::eDefault;
}

backend::config::SpaceFilling getSFC(std::string_view sfc)
{
    if (sfc == "morton32")
        return backend::config::SpaceFilling::eMorton32;
    return backend::config::SpaceFilling::eMorton32;
}

backend::config::InitialClusters getInitialClusters(std::string_view ic)
{
    static_cast<void>(ic);
    return backend::config::InitialClusters::eTriangles;
}

void getPipeline(toml::table const& tPipeline, toml::table const& tShaders, backend::config::BVHPipeline& pipeline)
{
    auto const getShader = [&](std::string_view key, std::string& shaderName) {
        if (auto const shader { tPipeline.at_path(key).value<std::string_view>() }; shader) {
            if (auto const value { tShaders.at_path(shader.value()).value<std::string_view>() }; value)
                shaderName = std::string(value.value());
            else
                berry::log::error("Shader with key '{}' not found.", shader.value());
        }
    };

    if (auto const value { tPipeline.at_path("plocpp.bv").value<std::string_view>() }; value)
        pipeline.plocpp.bv = getBoundingVolume(value.value());
    getShader("plocpp.shader.initial_clusters", pipeline.plocpp.shader.initialClusters);
    getShader("plocpp.shader.iterations", pipeline.plocpp.shader.iterations);

    if (auto const value { tPipeline.at_path("plocpp.space_filling").value<std::string_view>() }; value)
        pipeline.plocpp.sfc = getSFC(value.value());
    if (auto const value { tPipeline.at_path("plocpp.initial_clusters").value<std::string_view>() }; value)
        pipeline.plocpp.ic = getInitialClusters(value.value());
    if (auto const value { tPipeline.at_path("plocpp.radius").value<u32>() }; value)
        pipeline.plocpp.radius = value.value();

    if (auto const value { tPipeline.at_path("collapsing.bv").value<std::string_view>() }; value)
        pipeline.collapsing.bv = getBoundingVolume(value.value());
    getShader("collapsing.shader.collapse", pipeline.collapsing.shader.collapse);

    if (auto const value { tPipeline.at_path("collapsing.max_leaf_size").value<u32>() }; value)
        pipeline.collapsing.maxLeafSize = value.value();
    if (auto const value { tPipeline.at_path("collapsing.c_t").value<f32>() }; value)
        pipeline.collapsing.c_t = value.value();
    if (auto const value { tPipeline.at_path("collapsing.c_i").value<f32>() }; value)
        pipeline.collapsing.c_i = value.value();

    if (auto const value { tPipeline.at_path("transformation.bv").value<std::string_view>() }; value)
        pipeline.transformation.bv = getBoundingVolume(value.value());
    getShader("transformation.shader.transform", pipeline.transformation.shader.transform);

    if (auto const value { tPipeline.at_path("rearrangement.bv").value<std::string_view>() }; value)
        pipeline.rearrangement.bv = getBoundingVolume(value.value());
    getShader("rearrangement.shader.rearrange", pipeline.rearrangement.shader.rearrange);

    if (auto const value { tPipeline.at_path("rearrangement.layout").value<std::string_view>() }; value)
        pipeline.rearrangement.layout = getNodeLayout(value.value());

    if (auto const value { tPipeline.at_path("stats.c_t").value<f32>() }; value)
        pipeline.stats.c_t = value.value();
    if (auto const value { tPipeline.at_path("stats.c_i").value<f32>() }; value)
        pipeline.stats.c_i = value.value();

    if (auto const value { tPipeline.at_path("tracer.bv").value<std::string_view>() }; value)
        pipeline.tracer.bv = getBoundingVolume(value.value());
    getShader("tracer.shader.gen_primary", pipeline.tracer.shader.genPrimary);
    getShader("tracer.shader.trace_rays", pipeline.tracer.shader.traceRays);
    getShader("tracer.shader.shade_and_cast", pipeline.tracer.shader.shadeAndCast);
    getShader("tracer.shader.trace_bv", pipeline.tracer.shader.traceRays_bv);
    getShader("tracer.shader.shade_and_cast_bv", pipeline.tracer.shader.shadeAndCast_bv);
    getShader("tracer.shader.trace_int", pipeline.tracer.shader.traceRays_int);
    getShader("tracer.shader.shade_and_cast_int", pipeline.tracer.shader.shadeAndCast_int);

    if (auto const value { tPipeline.at_path("tracer.rays_primary.workgroup_count").value<u32>() }; value)
        pipeline.tracer.rPrimary.workgroupCount = value.value();
    if (auto const value { tPipeline.at_path("tracer.rays_primary.warps_per_workgroup").value<u32>() }; value)
        pipeline.tracer.rPrimary.warpsPerWorkgroup = value.value();
    if (auto const value { tPipeline.at_path("tracer.rays_secondary.workgroup_count").value<u32>() }; value)
        pipeline.tracer.rSecondary.workgroupCount = value.value();
    if (auto const value { tPipeline.at_path("tracer.rays_secondary.warps_per_workgroup").value<u32>() }; value)
        pipeline.tracer.rSecondary.warpsPerWorkgroup = value.value();
}
