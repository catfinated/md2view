#include "fixtures.hpp"
#include "md2view/image.hpp"
#include "md2view/pak.hpp"
#include "md2view/pcx.hpp"
#include "tmpdir.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <utility>

namespace {

using Pixel = std::array<unsigned char, 4>;

constexpr Pixel kRed{255, 0, 0, 255};
constexpr Pixel kBlue{0, 0, 255, 255};

Pixel pixelAt(Image const& image, unsigned int x, unsigned int y) {
    auto const data = image.data();
    auto const offset = ((std::size_t{y} * image.width()) + x) * 4;
    return {data[offset], data[offset + 1], data[offset + 2], data[offset + 3]};
}

// minimal.pcx and palette.png share this 2x2 layout:
//   (0,0)=red  (1,0)=blue
//   (0,1)=blue (1,1)=red
void requireRedBlueChecker(Image const& image) {
    REQUIRE(image.width() == 2);
    REQUIRE(image.height() == 2);
    REQUIRE(image.data().size() == 16); // 2 * 2 * 4 channels
    REQUIRE(pixelAt(image, 0, 0) == kRed);
    REQUIRE(pixelAt(image, 1, 0) == kBlue);
    REQUIRE(pixelAt(image, 0, 1) == kBlue);
    REQUIRE(pixelAt(image, 1, 1) == kRed);
}

} // namespace

// a span from a temporary Image would dangle, so data() is lvalue-only. A
// concept is needed: outside a template, a requires-expression with an
// invalid expression is a hard error rather than false.
template <typename T>
concept HasData = requires(T&& image) { std::forward<T>(image).data(); };
static_assert(HasData<Image const&>);
static_assert(HasData<Image&>);
static_assert(!HasData<Image>);

TEST_CASE("image from pcx", "[image]") {
    std::ifstream f(test_fixtures_dir() / "minimal.pcx", std::ios::binary);
    REQUIRE(f.is_open());
    Image const image{PCX{f}};

    requireRedBlueChecker(image);
}

TEST_CASE("image from palette png expands to rgba", "[image]") {
    // stb_image reports 3 channels in the file for a palette PNG; the image
    // must still load as RGBA with opaque alpha (regression: drfreak.png)
    Image const image{test_fixtures_dir() / "palette.png"};

    requireRedBlueChecker(image);
}

TEST_CASE("image from rgba png keeps alpha", "[image]") {
    Image const image{test_fixtures_dir() / "rgba.png"};

    REQUIRE(image.width() == 2);
    REQUIRE(image.height() == 2);
    REQUIRE(image.data().size() == 16);
    REQUIRE(pixelAt(image, 0, 0) == Pixel{255, 0, 0, 255});
    REQUIRE(pixelAt(image, 1, 0) == Pixel{0, 255, 0, 128});
    REQUIRE(pixelAt(image, 0, 1) == Pixel{0, 0, 255, 64});
    REQUIRE(pixelAt(image, 1, 1) == Pixel{255, 255, 255, 0});
}

TEST_CASE("image from missing file throws", "[image]") {
    auto construct = []() { Image{test_fixtures_dir() / "nosuchfile.png"}; };
    REQUIRE_THROWS_WITH(construct(), Catch::Matchers::ContainsSubstring(
                                         "failed to load image"));
}

TEST_CASE("image from invalid file throws", "[image]") {
    TmpDir tmp_dir;
    auto const path = tmp_dir.path() / "garbage.png";
    {
        std::ofstream f(path, std::ios::binary);
        f << "this is not an image";
    }
    auto construct = [&]() { Image{path}; };
    REQUIRE_THROWS_AS(construct(), std::runtime_error);
}

TEST_CASE("image load pcx from pak archive", "[image]") {
    PAK const pak{test_fixtures_dir() / "skin.pak"};
    REQUIRE_FALSE(pak.is_directory());

    auto const image = Image::loadFromPak(pak, "models/test/skin.pcx");

    requireRedBlueChecker(image);
}

TEST_CASE("image load from directory pak", "[image]") {
    TmpDir tmp_dir;
    auto const skins = tmp_dir.path() / "models" / "test";
    std::filesystem::create_directories(skins);

    SECTION("pcx") {
        std::filesystem::copy_file(test_fixtures_dir() / "minimal.pcx",
                                   skins / "skin.pcx");
        PAK const pak{tmp_dir.path()};

        requireRedBlueChecker(Image::loadFromPak(pak, "models/test/skin.pcx"));
    }

    SECTION("png") {
        std::filesystem::copy_file(test_fixtures_dir() / "palette.png",
                                   skins / "skin.png");
        PAK const pak{tmp_dir.path()};

        requireRedBlueChecker(Image::loadFromPak(pak, "models/test/skin.png"));
    }
}

TEST_CASE("image copy and move", "[image]") {
    Image const original{test_fixtures_dir() / "palette.png"};

    Image copy{original};
    REQUIRE(std::ranges::equal(copy.data(), original.data()));

    Image const moved{std::move(copy)};
    requireRedBlueChecker(moved);
    requireRedBlueChecker(original);
}
