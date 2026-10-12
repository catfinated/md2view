#include "fixtures.hpp"
#include "md2view/pak.hpp"
#include "tmpdir.hpp"

#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <ios>
#include <stdexcept>
#include <string>

TEST_CASE("pak non-existent", "[pak]") {
    auto construct = [&] { PAK pak{"nosuchfile.pak"}; };
    REQUIRE_THROWS_AS(construct(), std::runtime_error);
}

TEST_CASE("pak from directory", "[pak]") {
    TmpDir tmpDir;
    PAK pak{tmpDir.path()};

    REQUIRE(pak.isDirectory());
    REQUIRE(pak.fpath() == tmpDir.path());
    REQUIRE_FALSE(pak.hasModels());
}

TEST_CASE("pak from pak file is not directory", "[pak]") {
    auto path = testFixturesDir() / "minimal.pak";
    PAK pak{path};

    REQUIRE_FALSE(pak.isDirectory());
    REQUIRE(pak.fpath() == path);
}

TEST_CASE("pak from pak file has models", "[pak]") {
    auto path = testFixturesDir() / "minimal.pak";
    PAK pak{path};

    REQUIRE(pak.hasModels());
}

TEST_CASE("pak from pak file invalid magic throws", "[pak]") {
    TmpDir tmpDir;
    auto badPak = tmpDir.path() / "bad.pak";
    {
        std::ofstream f(badPak, std::ios::binary);
        f.write("NOPE\0\0\0\0\0\0\0\0", 12);
    }
    auto construct = [&] { PAK pak{badPak}; };
    REQUIRE_THROWS_AS(construct(), std::runtime_error);
}

TEST_CASE("pak openIfstream returns correct content", "[pak]") {
    auto path = testFixturesDir() / "minimal.pak";
    PAK pak{path};

    auto inf = pak.openIfstream("models/player/tris.md2");
    REQUIRE(inf.is_open());

    std::string content(5, '\0');
    inf.read(content.data(), 5);
    REQUIRE(content == "HELLO");
}
