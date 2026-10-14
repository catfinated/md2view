#pragma once

#include "md2view/asset_loader.hpp"
#include "md2view/gl/texture2d.hpp"

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

/// Cache and factory for GPU textures.
///
/// Caches loaded textures by
/// their path strings. Repeated calls for the same path return the cached
/// instance without re-reading from disk or re-uploading to the GPU.
class ResourceManager {
public:
    /// @param assets Source of texture images. Must outlive this object.
    explicit ResourceManager(AssetLoader& assets);
    ~ResourceManager() = default;

    ResourceManager(ResourceManager const&) = delete;
    ResourceManager& operator=(ResourceManager const&) = delete;

    ResourceManager(ResourceManager&&) noexcept = delete;
    ResourceManager& operator=(ResourceManager&&) noexcept = delete;

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
    AssetLoader& assets_;
    std::unordered_map<std::string, std::shared_ptr<GL::Texture2D>> textures2D_;
};
