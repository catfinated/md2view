#include "resource_manager.hpp"
#include "pcx.hpp"

#include <spdlog/spdlog.h>
#include <gsl-lite/gsl-lite.hpp>

#include <filesystem>

ResourceManager::ResourceManager(std::filesystem::path const& rootdir)
    : root_dir_(rootdir)
    , models_dir_(root_dir_ / "models")
    , shaders_dir_(root_dir_ / "shaders")
{
}

std::shared_ptr<Shader> ResourceManager::load_shader(std::string const& name,
                                                     std::string_view vertex,
                                                     std::string_view fragment,
                                                     std::optional<std::string_view> geometry)
{
    spdlog::info("loading shader {}", name);
    gsl_Assert(shaders_.find(name) == shaders_.end());

    auto vertex_path = shaders_dir() / std::string{vertex};
    auto fragment_path = shaders_dir() / std::string{fragment};
    std::optional<std::filesystem::path> geometry_path;
    if (geometry) {
        geometry_path = shaders_dir() / std::string{*geometry};
    }

    std::shared_ptr<Shader> shader{new Shader{vertex_path, fragment_path, geometry_path}};
    auto result = shaders_.emplace(std::make_pair(name, shader));

    gsl_Assert(result.second);
    spdlog::info("loaded shader {}", name);
    return result.first->second;
}

std::shared_ptr<Texture2D> ResourceManager::load_texture2D(PAK const& pf, std::string const& path, std::optional<std::string> const& name)
{
    auto key = name ? *name : path;
    auto iter = textures2D_.find(key);

    if (iter != textures2D_.end()) {
        return iter->second;
    }

    auto result = textures2D_.emplace(std::make_pair(key, Texture2D::load(pf, path)));
    return result.first->second;
}
