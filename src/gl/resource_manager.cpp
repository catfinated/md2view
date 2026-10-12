#include "md2view/gl/resource_manager.hpp"
#include "md2view/asset_loader.hpp"
#include "md2view/gl/shader.hpp"
#include "md2view/gl/texture2d.hpp"

#include <fmt/format.h>
#include <gsl-lite/gsl-lite.hpp>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

ResourceManager::ResourceManager(
    std::filesystem::path const& rootdir,
    std::optional<std::filesystem::path> const& pakPath)
    : AssetLoader(rootdir, pakPath)
    , shadersDir_(dataDir() / "shaders") {}

std::shared_ptr<GL::Shader>
ResourceManager::loadShader(std::string const& name,
                            std::optional<std::string_view> vertex,
                            std::optional<std::string_view> fragment,
                            std::optional<std::string_view> geometry) {
    gsl_Assert(!shaders_.contains(name));
    auto const vfname =
        vertex ? fmt::format("{}.vert", *vertex) : fmt::format("{}.vert", name);
    auto const ffname = fragment ? fmt::format("{}.frag", *fragment)
                                 : fmt::format("{}.frag", name);
    spdlog::info("loading shader {} {} {}", name, vfname, ffname);

    auto vertexPath = shadersDir() / vfname;
    auto fragmentPath = shadersDir() / ffname;
    std::optional<std::filesystem::path> geometryPath;
    if (geometry) {
        geometryPath = shadersDir() / std::string{*geometry};
    }

    auto shader =
        std::make_shared<GL::Shader>(vertexPath, fragmentPath, geometryPath);
    auto result = shaders_.emplace(name, shader);

    gsl_Assert(result.second);
    spdlog::info("loaded shader {}", name);
    return result.first->second;
}

std::shared_ptr<GL::Texture2D>
ResourceManager::loadTexture2D(std::string const& path,
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
