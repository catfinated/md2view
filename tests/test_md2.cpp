#include "fixtures.hpp"
#include "md2view/md2.hpp"
#include "md2view/pak.hpp"
#include "tmpdir.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <gsl-lite/gsl-lite.hpp>

#include <filesystem>
#include <fstream>
#include <stdexcept>

// Load the minimal.md2 fixture via a directory-mode PAK.
// The fixture lives at <fixtures>/minimal.md2; we expose it under the path
// "models/player/tris.md2" by symlinking into a temp directory tree.
static MD2 load_fixture() {
    auto fixture = test_fixtures_dir() / "minimal.md2";
    static TmpDir tmp;
    auto model_dir = tmp.path() / "models" / "player";
    std::filesystem::create_directories(model_dir);
    auto dest = model_dir / "tris.md2";
    if (!std::filesystem::exists(dest)) {
        std::filesystem::copy_file(fixture, dest);
    }
    PAK pak{tmp.path()};
    return MD2{"models/player/tris.md2", pak};
}

TEST_CASE("md2 non-existent", "[md2]") {
    TmpDir tmp_dir;
    PAK pak{tmp_dir.path()};
    auto construct = [&]() { MD2{"notafile.md2", pak}; };
    REQUIRE_THROWS_AS(construct(), gsl_lite::fail_fast);
}

TEST_CASE("md2 valid header fields", "[md2]") {
    auto md2 = load_fixture();
    auto const& hdr = md2.header();

    REQUIRE(hdr.ident == 844121161);
    REQUIRE(hdr.version == 8);
    REQUIRE(hdr.num_xyz == 3);
    REQUIRE(hdr.num_st == 3);
    REQUIRE(hdr.num_tris == 1);
    REQUIRE(hdr.num_frames == 1);
    REQUIRE(hdr.num_skins == 0);
}

TEST_CASE("md2 animation parsed from frame name", "[md2]") {
    auto md2 = load_fixture();

    REQUIRE(md2.animations().size() == 1);
    REQUIRE(md2.animations()[0].name == "stand");
    REQUIRE(md2.animations()[0].start_frame == 0);
    REQUIRE(md2.animations()[0].end_frame == 0);
}

TEST_CASE("md2 interpolated vertex count", "[md2]") {
    auto md2 = load_fixture();
    // num_tris * 3 vertices unpacked into flat buffer
    REQUIRE(md2.interpolated_vertices().size() == 3);
}

TEST_CASE("md2 interpolated vertex positions", "[md2]") {
    using Catch::Approx;
    auto md2 = load_fixture();
    auto const& verts = md2.interpolated_vertices();

    // Loader maps: v[0]→x, v[1]→z, v[2]→y; scale=(1,1,1), translate=(0,0,0)
    // v0=(0,0,0), v1=(1,0,0), v2=(0,0,1) in the file → vec3(x,y,z):
    REQUIRE(verts[0].x == Approx(0.0f));
    REQUIRE(verts[0].y == Approx(0.0f));
    REQUIRE(verts[0].z == Approx(0.0f));

    REQUIRE(verts[1].x == Approx(1.0f));
    REQUIRE(verts[1].y == Approx(0.0f));
    REQUIRE(verts[1].z == Approx(0.0f));

    REQUIRE(verts[2].x == Approx(0.0f));
    REQUIRE(verts[2].y == Approx(1.0f));
    REQUIRE(verts[2].z == Approx(0.0f));
}

TEST_CASE("md2 scaled texcoord count", "[md2]") {
    auto md2 = load_fixture();
    REQUIRE(md2.scaled_texcoords().size() == 3);
}

TEST_CASE("md2 scaled texcoord values", "[md2]") {
    using Catch::Approx;
    auto md2 = load_fixture();
    auto const& tc = md2.scaled_texcoords();

    // skinwidth=2, skinheight=2; raw texcoords: (0,0),(1,0),(0,1)
    REQUIRE(tc[0].x == Approx(0.0f));
    REQUIRE(tc[0].y == Approx(0.0f));

    REQUIRE(tc[1].x == Approx(0.5f));
    REQUIRE(tc[1].y == Approx(0.0f));

    REQUIRE(tc[2].x == Approx(0.0f));
    REQUIRE(tc[2].y == Approx(0.5f));
}

TEST_CASE("md2 update does not crash on single-frame animation", "[md2]") {
    auto md2 = load_fixture();
    // Single-frame animation — update is a no-op but must not crash
    md2.update(1.0f / 30.0f);
    REQUIRE(md2.interpolated_vertices().size() == 3);
}
