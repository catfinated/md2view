#include "md2view/pak.hpp"
#include "tmpdir.hpp"

#include <catch2/catch_test_macros.hpp>

#include <stdexcept>

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
