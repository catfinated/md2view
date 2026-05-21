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

// Load an md2 fixture by filename via a directory-mode PAK.
// The file is copied into a per-fixture static TmpDir as "tris.md2".
static MD2 load_md2_fixture(char const* fixture_name,
                            std::filesystem::path const& tmp_root) {
    auto model_dir = tmp_root / "models" / "player";
    std::filesystem::create_directories(model_dir);
    auto dest = model_dir / "tris.md2";
    if (!std::filesystem::exists(dest)) {
        std::filesystem::copy_file(test_fixtures_dir() / fixture_name, dest);
    }
    PAK pak{tmp_root};
    return MD2{"models/player/tris.md2", pak};
}

static MD2 load_fixture() {
    static TmpDir tmp;
    return load_md2_fixture("minimal.md2", tmp.path());
}

static MD2 load_two_frame() {
    static TmpDir tmp;
    return load_md2_fixture("two_frame.md2", tmp.path());
}

static MD2 load_two_anim() {
    static TmpDir tmp;
    return load_md2_fixture("two_anim.md2", tmp.path());
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

// --- set_frames_per_second ---

TEST_CASE("md2 frames per second default", "[md2]") {
    auto md2 = load_fixture();
    REQUIRE(md2.frames_per_second() == Catch::Approx(8.0f));
}

TEST_CASE("md2 set frames per second clamped to max", "[md2]") {
    auto md2 = load_fixture();
    md2.set_frames_per_second(999.0f);
    REQUIRE(md2.frames_per_second() == Catch::Approx(60.0f));
}

TEST_CASE("md2 set frames per second clamped to zero", "[md2]") {
    auto md2 = load_fixture();
    md2.set_frames_per_second(-1.0f);
    REQUIRE(md2.frames_per_second() == Catch::Approx(0.0f));
}

// --- default indices ---

TEST_CASE("md2 default animation index", "[md2]") {
    auto md2 = load_fixture();
    REQUIRE(md2.animation_index() == 0);
}

TEST_CASE("md2 default skin index", "[md2]") {
    auto md2 = load_fixture();
    REQUIRE(md2.skin_index() == 0);
}

// --- set_animation / set_skin_index error cases ---

TEST_CASE("md2 set animation unknown name is no-op", "[md2]") {
    auto md2 = load_fixture();
    md2.set_animation("does_not_exist");
    REQUIRE(md2.animation_index() == 0);
}

TEST_CASE("md2 set animation index out of range throws", "[md2]") {
    auto md2 = load_fixture();
    REQUIRE_THROWS_AS(md2.set_animation(size_t{999}), gsl_lite::fail_fast);
}

TEST_CASE("md2 set skin index out of range throws", "[md2]") {
    auto md2 = load_fixture();
    // fixture has 0 skins, so any index is out of range
    REQUIRE_THROWS_AS(md2.set_skin_index(size_t{0}), gsl_lite::fail_fast);
}

// --- two-frame animation (two_frame.md2) ---
// Frame 0 "stand0": world vertices (0,0,0),(2,0,0),(0,2,0)
// Frame 1 "stand1": world vertices (10,0,0),(12,0,0),(10,10,0)

TEST_CASE("md2 two-frame animation end frame", "[md2]") {
    auto md2 = load_two_frame();
    REQUIRE(md2.animations().size() == 1);
    REQUIRE(md2.animations()[0].start_frame == 0);
    REQUIRE(md2.animations()[0].end_frame == 1);
}

TEST_CASE("md2 update paused does not advance vertices", "[md2]") {
    using Catch::Approx;
    auto md2 = load_two_frame();
    md2.set_frames_per_second(0.0f);
    auto const x_before = md2.interpolated_vertices()[0].x;
    md2.update(1.0f);
    REQUIRE(md2.interpolated_vertices()[0].x == Approx(x_before));
}

TEST_CASE("md2 update interpolates between frames", "[md2]") {
    using Catch::Approx;
    auto md2 = load_two_frame();
    // fps=8 (default), dt=1/16 → interpolation_=0.5, no frame advance
    // lerp(frame0.v0=(0,0,0), frame1.v0=(10,0,0), 0.5) = (5,0,0)
    md2.update(1.0f / 16.0f);
    REQUIRE(md2.interpolated_vertices()[0].x == Approx(5.0f).margin(1e-4f));
}

TEST_CASE("md2 update advances to next frame", "[md2]") {
    using Catch::Approx;
    auto md2 = load_two_frame();
    // fps=8 (default), dt=1/8 → interpolation_=1.0 → advance to frame 1
    // lerp(frame1.v0=(10,0,0), frame0.v0=(0,0,0), 0.0) = (10,0,0)
    md2.update(1.0f / 8.0f);
    REQUIRE(md2.interpolated_vertices()[0].x == Approx(10.0f).margin(1e-4f));
}

// --- two-animation model (two_anim.md2) ---
// Frames: "stand0","stand1","run0","run1" → animations "stand"(0-1), "run"(2-3)

TEST_CASE("md2 multiple animations count", "[md2]") {
    auto md2 = load_two_anim();
    REQUIRE(md2.animations().size() == 2);
}

TEST_CASE("md2 multiple animations frame ranges", "[md2]") {
    auto md2 = load_two_anim();
    REQUIRE(md2.animations()[0].name == "stand");
    REQUIRE(md2.animations()[0].start_frame == 0);
    REQUIRE(md2.animations()[0].end_frame == 1);
    REQUIRE(md2.animations()[1].name == "run");
    REQUIRE(md2.animations()[1].start_frame == 2);
    REQUIRE(md2.animations()[1].end_frame == 3);
}

TEST_CASE("md2 set animation by name", "[md2]") {
    auto md2 = load_two_anim();
    md2.set_animation("run");
    REQUIRE(md2.animation_index() == 1);
}

TEST_CASE("md2 set animation by index", "[md2]") {
    auto md2 = load_two_anim();
    md2.set_animation("run");
    REQUIRE(md2.animation_index() == 1);
    md2.set_animation(size_t{0});
    REQUIRE(md2.animation_index() == 0);
}
