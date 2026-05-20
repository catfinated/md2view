#include "fixtures.hpp"
#include "md2view/pcx.hpp"

#include <catch2/catch_test_macros.hpp>
#include <gsl-lite/gsl-lite.hpp>

#include <fstream>
#include <sstream>

TEST_CASE("pcx default ctor", "[pcx]") {
    PCX pcx;

    REQUIRE(pcx.image().empty());
    REQUIRE(pcx.colors().empty());
    REQUIRE(pcx.width() == 0);
    REQUIRE(pcx.height() == 0);
}

TEST_CASE("pcx invalid istream", "[pcx]") {
    std::stringstream ss;
    auto construct = [&]() { PCX{ss}; };
    REQUIRE_THROWS_AS(construct(), gsl_lite::fail_fast);
}

TEST_CASE("pcx valid 2x2 dimensions", "[pcx]") {
    auto path = test_fixtures_dir() / "minimal.pcx";
    std::ifstream f(path, std::ios::binary);
    REQUIRE(f.is_open());
    PCX pcx{f};

    REQUIRE(pcx.width() == 2);
    REQUIRE(pcx.height() == 2);
    REQUIRE(pcx.image().size() == 12); // 2 * 2 * 3 channels
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

    // pixel (1,0) = palette index 1 = blue
    REQUIRE(img[3] == 0);
    REQUIRE(img[4] == 0);
    REQUIRE(img[5] == 255);

    // pixel (0,1) = palette index 1 = blue
    REQUIRE(img[6] == 0);
    REQUIRE(img[7] == 0);
    REQUIRE(img[8] == 255);

    // pixel (1,1) = palette index 0 = red
    REQUIRE(img[9] == 255);
    REQUIRE(img[10] == 0);
    REQUIRE(img[11] == 0);
}
