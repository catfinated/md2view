#include "fixtures.hpp"
#include "md2view/pak.hpp"
#include "tmpdir.hpp"

#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <stdexcept>
#include <string>

TEST_CASE("pak non-existent", "[pak]") {
    auto construct = [&]() { PAK pak{"nosuchfile.pak"}; };
    REQUIRE_THROWS_AS(construct(), std::runtime_error);
}

TEST_CASE("pak from directory", "[pak]") {
    TmpDir tmp_dir;
    PAK pak{tmp_dir.path()};

    REQUIRE(pak.is_directory());
    REQUIRE(pak.fpath() == tmp_dir.path());
    REQUIRE_FALSE(pak.has_models());
}

TEST_CASE("pak from pak file is not directory", "[pak]") {
    auto path = test_fixtures_dir() / "minimal.pak";
    PAK pak{path};

    REQUIRE_FALSE(pak.is_directory());
    REQUIRE(pak.fpath() == path);
}

TEST_CASE("pak from pak file has models", "[pak]") {
    auto path = test_fixtures_dir() / "minimal.pak";
    PAK pak{path};

    REQUIRE(pak.has_models());
}

TEST_CASE("pak from pak file invalid magic throws", "[pak]") {
    TmpDir tmp_dir;
    auto bad_pak = tmp_dir.path() / "bad.pak";
    {
        std::ofstream f(bad_pak, std::ios::binary);
        f.write("NOPE\0\0\0\0\0\0\0\0", 12);
    }
    auto construct = [&]() { PAK pak{bad_pak}; };
    REQUIRE_THROWS_AS(construct(), std::runtime_error);
}

TEST_CASE("pak open_ifstream returns correct content", "[pak]") {
    auto path = test_fixtures_dir() / "minimal.pak";
    PAK pak{path};

    auto inf = pak.open_ifstream("models/player/tris.md2");
    REQUIRE(inf.is_open());

    std::string content(5, '\0');
    inf.read(content.data(), 5);
    REQUIRE(content == "HELLO");
}
