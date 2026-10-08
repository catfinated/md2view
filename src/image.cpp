#include "md2view/image.hpp"

#include "md2view/pak.hpp"
#include "md2view/pcx.hpp"

#include <fmt/core.h>
#include <gsl-lite/gsl-lite.hpp>
#include <spdlog/spdlog.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <memory>

Image::Image(PCX&& pcx)
    : width_(gsl_lite::narrow_failfast<unsigned int>(pcx.width()))
    , height_(gsl_lite::narrow_failfast<unsigned int>(pcx.height()))
    , storage_(std::move(pcx).image()) {}

Image::Image(std::filesystem::path const& fpath) {
    int width{};
    int height{};
    int n{};
    constexpr int desiredChannels{4};

    auto data =
        stbi_load(fpath.string().c_str(), &width, &height, &n, desiredChannels);
    std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> ptr{data,
                                                             &stbi_image_free};

    if (!ptr) {
        throw std::runtime_error(fmt::format("failed to load image '{}': {}",
                                             fpath.string(),
                                             stbi_failure_reason()));
    }

    width_ = gsl_lite::narrow_failfast<unsigned int>(width);
    height_ = gsl_lite::narrow_failfast<unsigned int>(height);
    auto const size = std::size_t{width_} * height_ * desiredChannels;
    storage_.assign(ptr.get(), ptr.get() + size);
}

Image Image::loadFromPak(PAK const& pak, std::string_view path) {

    spdlog::info("load image {} from {}", path, pak.fpath().string());
    auto const isPcx = std::filesystem::path(path).extension() == ".pcx";

    if (isPcx) {
        auto inf = pak.open_ifstream(path);
        gsl_Assert(inf.is_open());
        return Image{PCX{inf}};
    }

    auto const abspath = (pak.fpath() / path).make_preferred();
    return Image{abspath};
}
