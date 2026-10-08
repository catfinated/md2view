#include "md2view/gl/resource_manager.hpp"

#include <gsl-lite/gsl-lite.hpp>
#include <spdlog/spdlog.h>

#include <filesystem>

ResourceManager::ResourceManager(
    std::filesystem::path const& rootdir,
    std::optional<std::filesystem::path> const& pak_path)
    : AssetLoader(rootdir, pak_path)
    , shaders_dir_(dataDir() / "shaders") {}

std::shared_ptr<GL::Shader>
ResourceManager::load_shader(std::string const& name,
                             std::optional<std::string_view> vertex,
                             std::optional<std::string_view> fragment,
                             std::optional<std::string_view> geometry) {
    gsl_Assert(!shaders_.contains(name));
    auto const vfname =
        vertex ? fmt::format("{}.vert", *vertex) : fmt::format("{}.vert", name);
    auto const ffname = fragment ? fmt::format("{}.frag", *fragment)
                                 : fmt::format("{}.frag", name);
    spdlog::info("loading shader {} {} {}", name, vfname, ffname);

    auto vertex_path = shaders_dir() / vfname;
    auto fragment_path = shaders_dir() / ffname;
    std::optional<std::filesystem::path> geometry_path;
    if (geometry) {
        geometry_path = shaders_dir() / std::string{*geometry};
    }

    std::shared_ptr<GL::Shader> shader{
        new GL::Shader{vertex_path, fragment_path, geometry_path}};
    auto result = shaders_.emplace(name, shader);

    gsl_Assert(result.second);
    spdlog::info("loaded shader {}", name);
    return result.first->second;
}

std::shared_ptr<GL::Texture2D>
ResourceManager::load_texture2D(std::string const& path,
                                std::optional<std::string> const& name) {
    auto key = name ? *name : path;
    auto iter = textures2D_.find(key);

    if (iter != textures2D_.end()) {
        return iter->second;
    }

    auto result = textures2D_.emplace(
        key, std::make_shared<GL::Texture2D>(loadImage(path)));
    return result.first->second;
}
