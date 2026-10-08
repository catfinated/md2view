#pragma once

#include <filesystem>
#include <span>
#include <string_view>
#include <vector>

class PAK;
class PCX;

/// An RGBA8 image
class Image {
public:
    explicit Image(PCX&& pcx);
    explicit Image(std::filesystem::path const& fpath);

    [[nodiscard]] unsigned int width() const { return width_; }
    [[nodiscard]] unsigned int height() const { return height_; }
    [[nodiscard]] std::span<unsigned char const> data() const& {
        return std::span{storage_};
    }
    std::span<unsigned char const> data() && = delete;

    static Image loadFromPak(PAK const& pak, std::string_view path);

private:
    unsigned int width_{};
    unsigned int height_{};
    std::vector<unsigned char> storage_;
};
