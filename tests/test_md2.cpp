#include "md2view/md2.hpp"
#include "md2view/pak.hpp"
#include "tmpdir.hpp"

#include <catch2/catch_test_macros.hpp>
#include <gsl-lite/gsl-lite.hpp>

#include <stdexcept>

TEST_CASE("md2 non-existent", "[md2]") {
    TmpDir tmp_dir;
    PAK pak{tmp_dir.path()};
    auto construct = [&]() { MD2{"notafile.md2", pak}; };
    REQUIRE_THROWS_AS(construct(), gsl_lite::fail_fast);
}
