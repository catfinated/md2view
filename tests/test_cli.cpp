#include "md2view/cli.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <spdlog/common.h>

#include <array>
#include <filesystem>
#include <sstream>

TEST_CASE("cli defaults", "[cli]") {
    std::array<char const*, 1> argv{"md2v"};
    std::ostringstream out;
    auto const args = parseCli(argv, out);
    REQUIRE(args);
    REQUIRE(args->width == 1280);
    REQUIRE(args->height == 800);
    REQUIRE_FALSE(args->pak);
    REQUIRE(args->logLevel == spdlog::level::info);
    REQUIRE(out.str().empty());
}

TEST_CASE("cli long options", "[cli]") {
    std::array<char const*, 9> argv{
        "md2v",  "--width",  "640",         "--height", "480",
        "--pak", "pak0.pak", "--log-level", "debug",
    };
    std::ostringstream out;
    auto const args = parseCli(argv, out);
    REQUIRE(args);
    REQUIRE(args->width == 640);
    REQUIRE(args->height == 480);
    REQUIRE(args->pak == std::filesystem::path{"pak0.pak"});
    REQUIRE(args->logLevel == spdlog::level::debug);
}

TEST_CASE("cli short options", "[cli]") {
    std::array<char const*, 9> argv{
        "md2v", "-W", "640", "-H", "480", "-p", "models", "-l", "off",
    };
    std::ostringstream out;
    auto const args = parseCli(argv, out);
    REQUIRE(args);
    REQUIRE(args->width == 640);
    REQUIRE(args->height == 480);
    REQUIRE(args->pak == std::filesystem::path{"models"});
    REQUIRE(args->logLevel == spdlog::level::off);
}

TEST_CASE("cli help prints usage", "[cli]") {
    std::array<char const*, 2> argv{"md2v", "--help"};
    std::ostringstream out;
    REQUIRE_FALSE(parseCli(argv, out));
    REQUIRE_THAT(out.str(), Catch::Matchers::ContainsSubstring("--pak"));
}

TEST_CASE("cli rejects invalid arguments", "[cli]") {
    std::ostringstream out;

    SECTION("unknown log level") {
        std::array<char const*, 3> argv{"md2v", "--log-level", "loud"};
        REQUIRE_FALSE(parseCli(argv, out));
        REQUIRE_THAT(out.str(), Catch::Matchers::ContainsSubstring("loud"));
    }
    SECTION("unknown option") {
        std::array<char const*, 2> argv{"md2v", "--bogus"};
        REQUIRE_FALSE(parseCli(argv, out));
        REQUIRE_THAT(out.str(), Catch::Matchers::ContainsSubstring("bogus"));
    }
    SECTION("non-numeric width") {
        std::array<char const*, 3> argv{"md2v", "--width", "wide"};
        REQUIRE_FALSE(parseCli(argv, out));
        REQUIRE_FALSE(out.str().empty());
    }
    SECTION("missing value") {
        std::array<char const*, 2> argv{"md2v", "--pak"};
        REQUIRE_FALSE(parseCli(argv, out));
        REQUIRE_FALSE(out.str().empty());
    }
}
