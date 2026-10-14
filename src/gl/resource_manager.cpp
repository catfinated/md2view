#include "md2view/gl/resource_manager.hpp"
#include "md2view/asset_loader.hpp"
#include "md2view/gl/texture2d.hpp"

#include <memory>
#include <optional>
#include <string>

ResourceManager::ResourceManager(AssetLoader& assets)
    : assets_(assets) {}

std::shared_ptr<GL::Texture2D>
ResourceManager::loadTexture2D(std::string const& path,
                               std::optional<std::string> const& name) {
    auto key = name ? *name : path;
    auto iter = textures2D_.find(key);

    if (iter != textures2D_.end()) {
        return iter->second;
    }

    auto result = textures2D_.emplace(
        key, std::make_shared<GL::Texture2D>(assets_.loadImage(path)));
    return result.first->second;
}
