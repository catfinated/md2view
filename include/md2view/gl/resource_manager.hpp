#pragma once

#include "md2view/asset_loader.hpp"
#include "md2view/gl/shader.hpp"
#include "md2view/gl/texture2d.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

/// Cache and factory for GPU resources loaded from a PAK archive.
///
/// Caches loaded shaders and textures by
/// their path strings. Repeated calls for the same path return the cached
/// instance without re-reading from disk or re-uploading to the GPU.
class ResourceManager : public AssetLoader {
public:
    /// @param rootdir  Project data root (shaders are expected at
    /// `rootdir/shaders/`).
    /// @param pakPath Path to a `.pak` file or directory. Defaults to
    /// `rootdir/models`.
    explicit ResourceManager(
        std::filesystem::path const& rootdir,
        std::optional<std::filesystem::path> const& pakPath = std::nullopt);

    [[nodiscard]] std::filesystem::path const& shadersDir() const {
        return shadersDir_;
    }

    /// Compile and cache a shader program.
    ///
    /// Each name must be unique; calling this twice with the same name asserts.
    ///
    /// @param name     Cache key and default base name for the shader files.
    /// @param vertex   Override for the vertex shader file stem (defaults to @p
    /// name).
    /// @param fragment Override for the fragment shader file stem (defaults to
    /// @p name).
    /// @param geometry Optional geometry shader file name.
    std::shared_ptr<GL::Shader>
    loadShader(std::string const& name,
               std::optional<std::string_view> vertex = {},
               std::optional<std::string_view> fragment = {},
               std::optional<std::string_view> geometry = {});

    /// Return a previously loaded shader by name.
    std::shared_ptr<GL::Shader> shader(std::string const& name) {
        return shaders_.at(name);
    }

    /// Load and cache a texture from the active PAK.
    ///
    /// If @p name is provided it is used as the cache key; otherwise @p path
    /// is. Returns the cached instance if already loaded.
    std::shared_ptr<GL::Texture2D>
    loadTexture2D(std::string const& path,
                  std::optional<std::string> const& name = {});

    std::shared_ptr<GL::Texture2D> texture2D(std::string const& name) {
        return textures2D_.at(name);
    }

private:
    std::filesystem::path shadersDir_;
    std::unordered_map<std::string, std::shared_ptr<GL::Shader>> shaders_;
    std::unordered_map<std::string, std::shared_ptr<GL::Texture2D>> textures2D_;
};
