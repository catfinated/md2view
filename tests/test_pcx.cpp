#include "fixtures.hpp"
#include "md2view/pak.hpp"
#include "md2view/pcx.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <gsl-lite/gsl-lite.hpp>

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ios>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

std::string minimal_pcx_bytes() {
    std::ifstream f(test_fixtures_dir() / "minimal.pcx", std::ios::binary);
    return {std::istreambuf_iterator<char>(f),
            std::istreambuf_iterator<char>()};
}

void set_u16(std::string& bytes, std::size_t offset, uint16_t value) {
    bytes.at(offset) = static_cast<char>(value & 0xFF);
    bytes.at(offset + 1) = static_cast<char>(value >> 8);
}

// header field offsets, see PCX::Header
constexpr std::size_t kEncodingOffset = 2;
constexpr std::size_t kBitsPerPixelOffset = 3;
constexpr std::size_t kXStartOffset = 4;
constexpr std::size_t kNumBitPlanesOffset = 65;
constexpr std::size_t kBytesPerLineOffset = 66;
constexpr std::size_t kHeaderSize = 128;
// minimal.pcx: header, 4 bytes of image data, then the palette marker
constexpr std::size_t kPaletteMarkerOffset = kHeaderSize + 4;

void require_malformed(std::string const& bytes, std::string const& message) {
    std::istringstream is{bytes};
    REQUIRE_THROWS_MATCHES(PCX{is}, std::runtime_error,
                           Catch::Matchers::MessageMatches(
                               Catch::Matchers::ContainsSubstring(message)));
}

} // namespace

TEST_CASE("pcx default ctor", "[pcx]") {
    PCX pcx;

    REQUIRE(pcx.image().empty());
    REQUIRE(pcx.colors().empty());
    REQUIRE(pcx.width() == 0);
    REQUIRE(pcx.height() == 0);
}

TEST_CASE("pcx invalid istream", "[pcx]") {
    std::stringstream ss;
    auto construct = [&] { PCX{ss}; };
    REQUIRE_THROWS_AS(construct(), gsl_lite::fail_fast);
}

TEST_CASE("pcx valid 2x2 dimensions", "[pcx]") {
    auto path = test_fixtures_dir() / "minimal.pcx";
    std::ifstream f(path, std::ios::binary);
    REQUIRE(f.is_open());
    PCX pcx{f};

    REQUIRE(pcx.width() == 2);
    REQUIRE(pcx.height() == 2);
    REQUIRE(pcx.image().size() == 16); // 2 * 2 * 4 channels
}

TEST_CASE("pcx valid 2x2 palette", "[pcx]") {
    auto path = test_fixtures_dir() / "minimal.pcx";
    std::ifstream f(path, std::ios::binary);
    REQUIRE(f.is_open());
    PCX pcx{f};

    REQUIRE(pcx.colors().size() == 256);

    auto const& red = pcx.colors()[0];
    REQUIRE(red.r == 255);
    REQUIRE(red.g == 0);
    REQUIRE(red.b == 0);

    auto const& blue = pcx.colors()[1];
    REQUIRE(blue.r == 0);
    REQUIRE(blue.g == 0);
    REQUIRE(blue.b == 255);
}

TEST_CASE("pcx valid 2x2 pixels", "[pcx]") {
    auto path = test_fixtures_dir() / "minimal.pcx";
    std::ifstream f(path, std::ios::binary);
    REQUIRE(f.is_open());
    PCX pcx{f};

    auto const& img = pcx.image();

    // pixel (0,0) = palette index 0 = red
    REQUIRE(img[0] == 255);
    REQUIRE(img[1] == 0);
    REQUIRE(img[2] == 0);
    REQUIRE(img[3] == 255);

    // pixel (1,0) = palette index 1 = blue
    REQUIRE(img[4] == 0);
    REQUIRE(img[5] == 0);
    REQUIRE(img[6] == 255);
    REQUIRE(img[7] == 255);

    // pixel (0,1) = palette index 1 = blue
    REQUIRE(img[8] == 0);
    REQUIRE(img[9] == 0);
    REQUIRE(img[10] == 255);
    REQUIRE(img[11] == 255);

    // pixel (1,1) = palette index 0 = red
    REQUIRE(img[12] == 255);
    REQUIRE(img[13] == 0);
    REQUIRE(img[14] == 0);
    REQUIRE(img[15] == 255);
}

TEST_CASE("pcx invalid image size throws", "[pcx]") {
    auto bytes = minimal_pcx_bytes();
    set_u16(bytes, kXStartOffset, 5); // xstart > xend
    require_malformed(bytes, "invalid image size");
}

TEST_CASE("pcx scan line narrower than image throws", "[pcx]") {
    auto bytes = minimal_pcx_bytes();
    set_u16(bytes, kBytesPerLineOffset, 1); // image is 2 pixels wide
    require_malformed(bytes, "shorter than the image width");
}

TEST_CASE("pcx truncated image data throws", "[pcx]") {
    auto const bytes = minimal_pcx_bytes().substr(0, kHeaderSize + 1);
    require_malformed(bytes, "image data is truncated");
}

TEST_CASE("pcx short palette throws", "[pcx]") {
    auto bytes = minimal_pcx_bytes();
    bytes.resize(bytes.size() - 3); // drop the last palette entry
    require_malformed(bytes, "palette has 255 colors");
}

TEST_CASE("pcx bad palette marker throws", "[pcx]") {
    auto bytes = minimal_pcx_bytes();
    bytes.at(kPaletteMarkerOffset) = 0x00; // must be 0x0C
    require_malformed(bytes, "missing 256-colour palette");
}

TEST_CASE("pcx in a pak archive reads exactly 256 palette colors", "[pcx]") {
    PAK const pak{test_fixtures_dir() / "skin.pak"};
    auto in = pak.open_ifstream("models/test/skin.pcx");
    PCX const pcx{in};

    REQUIRE(pcx.colors().size() == 256);
    REQUIRE(pcx.colors()[0].r == 255); // index 0 = red
    REQUIRE(pcx.colors()[1].b == 255); // index 1 = blue
}

TEST_CASE("pcx 24-bit three plane image is unsupported", "[pcx]") {
    auto bytes = minimal_pcx_bytes();
    bytes.at(kNumBitPlanesOffset) = 3;
    require_malformed(bytes, "unsupported PCX: 3 colour plane(s)");
}

TEST_CASE("pcx non 8-bit image is unsupported", "[pcx]") {
    auto bytes = minimal_pcx_bytes();
    bytes.at(kBitsPerPixelOffset) = 4;
    require_malformed(bytes, "of 4 bits per pixel");
}

TEST_CASE("pcx uncompressed image is unsupported", "[pcx]") {
    auto bytes = minimal_pcx_bytes();
    bytes.at(kEncodingOffset) = 0;
    require_malformed(bytes, "unsupported PCX: encoding 0");
}
