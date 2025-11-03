#include "md2view/pcx.hpp"

#include <catch2/catch_test_macros.hpp>
#include <gsl-lite/gsl-lite.hpp>

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
