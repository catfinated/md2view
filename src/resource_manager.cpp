#include "resource_manager.hpp"
#include "pcx.hpp"

#include <gsl-lite/gsl-lite.hpp>
#include <spdlog/spdlog.h>

#include <filesystem>

ResourceManager::ResourceManager(
    std::filesystem::path const& rootdir,
    std::optional<std::filesystem::path> const& pak_path)
    : root_dir_(rootdir)
    , shaders_dir_(root_dir_ / "shaders")
    , pak_(std::make_unique<PAK>(pak_path.value_or(rootdir / "models"))) {}

std::shared_ptr<Shader>
ResourceManager::load_shader(std::string const& name,
                             std::optional<std::string_view> vertex,
                             std::optional<std::string_view> fragment,
                             std::optional<std::string_view> geometry) {
    gsl_Assert(shaders_.find(name) == shaders_.end());
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

    std::shared_ptr<Shader> shader{
        new Shader{vertex_path, fragment_path, geometry_path}};
    auto result = shaders_.emplace(name, shader);

    gsl_Assert(result.second);
    spdlog::info("loaded shader {}", name);
    return result.first->second;
}

std::shared_ptr<Texture2D>
ResourceManager::load_texture2D(std::string const& path,
                                std::optional<std::string> const& name) {
    auto key = name ? *name : path;
    auto iter = textures2D_.find(key);

    if (iter != textures2D_.end()) {
        return iter->second;
    }

    auto result = textures2D_.emplace(key, Texture2D::load(pak(), path));
    return result.first->second;
}

std::shared_ptr<MD2> ResourceManager::load_model(std::string const& path) {
    auto const iter = models_.find(path);
    if (iter != models_.end()) {
        return iter->second;
    }

    auto md2 = std::make_shared<MD2>(path, pak());
    auto result = models_.emplace(path, std::move(md2));
    return result.first->second;
}
