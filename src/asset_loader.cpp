#include "md2view/asset_loader.hpp"

AssetLoader::AssetLoader(std::filesystem::path const& dataDir,
                         std::optional<std::filesystem::path> const& pakPath)
    : dataDir_{dataDir}
    , pak_{pakPath.value_or(dataDir / "models")} {}

Image AssetLoader::loadImage(std::string_view path) {
    return Image::loadFromPak(pak_, path);
}

std::shared_ptr<MD2> AssetLoader::loadModel(std::string_view path) {
    auto const iter = models_.find(path);
    if (iter != models_.end()) {
        return iter->second;
    }

    auto result =
        models_.emplace(std::string{path}, std::make_shared<MD2>(path, pak_));
    return result.first->second;
}
