#pragma once

#include "md2view/image.hpp"
#include "md2view/md2.hpp"
#include "md2view/pak.hpp"

#include "md2view/detail/string_hash.hpp"

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

/// cpu asset loading
class AssetLoader {
public:
    /// @param dataDir Project data root
    /// @param pakPath Path to a `.pak` file or directory. Defaults to
    /// `dataDir/models`.
    explicit AssetLoader(
        std::filesystem::path const& dataDir,
        std::optional<std::filesystem::path> const& pakPath = std::nullopt);

    [[nodiscard]] std::filesystem::path const& dataDir() const {
        return dataDir_;
    }

    PAK const& pak() const { return pak_; }

    /// Load an image from the active PAK (no cache)
    Image loadImage(std::string_view path);

    /// Load and cache an MD2 model from the active PAK.
    ///
    /// Returns the cached instance if @p path was already loaded.
    std::shared_ptr<MD2> loadModel(std::string_view path);

private:
    std::filesystem::path dataDir_;
    PAK pak_;
    std::unordered_map<std::string,
                       std::shared_ptr<MD2>,
                       detail::StringHash,
                       std::equal_to<>>
        models_;
};
