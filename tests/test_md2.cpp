#include "fixtures.hpp"
#include "md2view/md2.hpp"
#include "md2view/pak.hpp"
#include "tmpdir.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <gsl-lite/gsl-lite.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <stdexcept>
#include <string>

namespace {

// Load an md2 fixture by filename via a directory-mode PAK.
// The file is copied into a per-fixture static TmpDir as "tris.md2".
MD2 loadMd2Fixture(char const* fixtureName,
                   std::filesystem::path const& tmpRoot) {
    auto modelDir = tmpRoot / "models" / "player";
    std::filesystem::create_directories(modelDir);
    auto dest = modelDir / "tris.md2";
    if (!std::filesystem::exists(dest)) {
        std::filesystem::copy_file(testFixturesDir() / fixtureName, dest);
    }
    PAK pak{tmpRoot};
    return MD2{"models/player/tris.md2", pak};
}

MD2 loadFixture() {
    static TmpDir tmp;
    return loadMd2Fixture("minimal.md2", tmp.path());
}

MD2 loadTwoFrame() {
    static TmpDir tmp;
    return loadMd2Fixture("two_frame.md2", tmp.path());
}

MD2 loadTwoAnim() {
    static TmpDir tmp;
    return loadMd2Fixture("two_anim.md2", tmp.path());
}

// minimal.md2's single triangle: three uint16 vertex indices followed by
// three uint16 texture coordinate indices (68-byte header + 3 texcoords * 4)
constexpr std::size_t kTriangleOffset = 80;
constexpr std::size_t kTriangleStOffset = kTriangleOffset + 6;

// Write minimal.md2 with one uint16 overwritten and try to load it.
void requireMalformedMd2(std::size_t offset,
                         uint16_t value,
                         std::string const& message) {
    std::ifstream in(testFixturesDir() / "minimal.md2", std::ios::binary);
    std::string bytes{std::istreambuf_iterator<char>(in),
                      std::istreambuf_iterator<char>()};
    bytes.at(offset) = static_cast<char>(value & 0xFF);
    bytes.at(offset + 1) = static_cast<char>(value >> 8);

    TmpDir tmp;
    auto const modelDir = tmp.path() / "models" / "player";
    std::filesystem::create_directories(modelDir);
    {
        std::ofstream out(modelDir / "tris.md2", std::ios::binary);
        out << bytes;
    }
    PAK const pak{tmp.path()};
    REQUIRE_THROWS_MATCHES(MD2("models/player/tris.md2", pak),
                           std::runtime_error,
                           Catch::Matchers::MessageMatches(
                               Catch::Matchers::ContainsSubstring(message)));
}

} // namespace

TEST_CASE("md2 non-existent", "[md2]") {
    TmpDir tmpDir;
    PAK pak{tmpDir.path()};
    auto construct = [&] { MD2{"notafile.md2", pak}; };
    REQUIRE_THROWS_AS(construct(), gsl_lite::fail_fast);
}

TEST_CASE("md2 valid header fields", "[md2]") {
    auto md2 = loadFixture();
    auto const& hdr = md2.header();

    REQUIRE(hdr.ident == 844121161);
    REQUIRE(hdr.version == 8);
    REQUIRE(hdr.numXyz == 3);
    REQUIRE(hdr.numSt == 3);
    REQUIRE(hdr.numTris == 1);
    REQUIRE(hdr.numFrames == 1);
    REQUIRE(hdr.numSkins == 0);
}

TEST_CASE("md2 animation parsed from frame name", "[md2]") {
    auto md2 = loadFixture();

    REQUIRE(md2.animations().size() == 1);
    REQUIRE(md2.animations()[0].name == "stand");
    REQUIRE(md2.animations()[0].startFrame == 0);
    REQUIRE(md2.animations()[0].endFrame == 0);
}

TEST_CASE("md2 interpolated vertex count", "[md2]") {
    auto md2 = loadFixture();
    // numTris * 3 vertices unpacked into flat buffer
    REQUIRE(md2.interpolatedVertices().size() == 3);
}

TEST_CASE("md2 interpolated vertex positions", "[md2]") {
    using Catch::Approx;
    auto md2 = loadFixture();
    auto const& verts = md2.interpolatedVertices();

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
    auto md2 = loadFixture();
    REQUIRE(md2.scaledTexcoords().size() == 3);
}

TEST_CASE("md2 scaled texcoord values", "[md2]") {
    using Catch::Approx;
    auto md2 = loadFixture();
    auto const& tc = md2.scaledTexcoords();

    // skinwidth=2, skinheight=2; raw texcoords: (0,0),(1,0),(0,1)
    REQUIRE(tc[0].x == Approx(0.0f));
    REQUIRE(tc[0].y == Approx(0.0f));

    REQUIRE(tc[1].x == Approx(0.5f));
    REQUIRE(tc[1].y == Approx(0.0f));

    REQUIRE(tc[2].x == Approx(0.0f));
    REQUIRE(tc[2].y == Approx(0.5f));
}

TEST_CASE("md2 update does not crash on single-frame animation", "[md2]") {
    auto md2 = loadFixture();
    // Single-frame animation — update is a no-op but must not crash
    md2.update(1.0f / 30.0f);
    REQUIRE(md2.interpolatedVertices().size() == 3);
}

// --- setFramesPerSecond ---

TEST_CASE("md2 frames per second default", "[md2]") {
    auto md2 = loadFixture();
    REQUIRE(md2.framesPerSecond() == Catch::Approx(8.0f));
}

TEST_CASE("md2 set frames per second clamped to max", "[md2]") {
    auto md2 = loadFixture();
    md2.setFramesPerSecond(999.0f);
    REQUIRE(md2.framesPerSecond() == Catch::Approx(60.0f));
}

TEST_CASE("md2 set frames per second clamped to zero", "[md2]") {
    auto md2 = loadFixture();
    md2.setFramesPerSecond(-1.0f);
    REQUIRE(md2.framesPerSecond() == Catch::Approx(0.0f));
}

// --- default indices ---

TEST_CASE("md2 default animation index", "[md2]") {
    auto md2 = loadFixture();
    REQUIRE(md2.animationIndex() == 0);
}

TEST_CASE("md2 default skin index", "[md2]") {
    auto md2 = loadFixture();
    REQUIRE(md2.skinIndex() == 0);
}

// --- setAnimation / setSkinIndex error cases ---

TEST_CASE("md2 set animation unknown name is no-op", "[md2]") {
    auto md2 = loadFixture();
    md2.setAnimation("does_not_exist");
    REQUIRE(md2.animationIndex() == 0);
}

TEST_CASE("md2 set animation index out of range throws", "[md2]") {
    auto md2 = loadFixture();
    REQUIRE_THROWS_AS(md2.setAnimation(size_t{999}), gsl_lite::fail_fast);
}

TEST_CASE("md2 set skin index out of range throws", "[md2]") {
    auto md2 = loadFixture();
    // fixture has 0 skins, so any index is out of range
    REQUIRE_THROWS_AS(md2.setSkinIndex(size_t{0}), gsl_lite::fail_fast);
}

// --- two-frame animation (two_frame.md2) ---
// Frame 0 "stand0": world vertices (0,0,0),(2,0,0),(0,2,0)
// Frame 1 "stand1": world vertices (10,0,0),(12,0,0),(10,10,0)

TEST_CASE("md2 two-frame animation end frame", "[md2]") {
    auto md2 = loadTwoFrame();
    REQUIRE(md2.animations().size() == 1);
    REQUIRE(md2.animations()[0].startFrame == 0);
    REQUIRE(md2.animations()[0].endFrame == 1);
}

TEST_CASE("md2 update paused does not advance vertices", "[md2]") {
    using Catch::Approx;
    auto md2 = loadTwoFrame();
    md2.setFramesPerSecond(0.0f);
    auto const xBefore = md2.interpolatedVertices()[0].x;
    md2.update(1.0f);
    REQUIRE(md2.interpolatedVertices()[0].x == Approx(xBefore));
}

TEST_CASE("md2 update interpolates between frames", "[md2]") {
    using Catch::Approx;
    auto md2 = loadTwoFrame();
    // fps=8 (default), dt=1/16 → interpolation_=0.5, no frame advance
    // lerp(frame0.v0=(0,0,0), frame1.v0=(10,0,0), 0.5) = (5,0,0)
    md2.update(1.0f / 16.0f);
    REQUIRE(md2.interpolatedVertices()[0].x == Approx(5.0f).margin(1e-4f));
}

TEST_CASE("md2 update advances to next frame", "[md2]") {
    using Catch::Approx;
    auto md2 = loadTwoFrame();
    // fps=8 (default), dt=1/8 → interpolation_=1.0 → advance to frame 1
    // lerp(frame1.v0=(10,0,0), frame0.v0=(0,0,0), 0.0) = (10,0,0)
    md2.update(1.0f / 8.0f);
    REQUIRE(md2.interpolatedVertices()[0].x == Approx(10.0f).margin(1e-4f));
}

// --- two-animation model (two_anim.md2) ---
// Frames: "stand0","stand1","run0","run1" → animations "stand"(0-1), "run"(2-3)

TEST_CASE("md2 multiple animations count", "[md2]") {
    auto md2 = loadTwoAnim();
    REQUIRE(md2.animations().size() == 2);
}

TEST_CASE("md2 multiple animations frame ranges", "[md2]") {
    auto md2 = loadTwoAnim();
    REQUIRE(md2.animations()[0].name == "stand");
    REQUIRE(md2.animations()[0].startFrame == 0);
    REQUIRE(md2.animations()[0].endFrame == 1);
    REQUIRE(md2.animations()[1].name == "run");
    REQUIRE(md2.animations()[1].startFrame == 2);
    REQUIRE(md2.animations()[1].endFrame == 3);
}

TEST_CASE("md2 set animation by name", "[md2]") {
    auto md2 = loadTwoAnim();
    md2.setAnimation("run");
    REQUIRE(md2.animationIndex() == 1);
}

TEST_CASE("md2 set animation by index", "[md2]") {
    auto md2 = loadTwoAnim();
    md2.setAnimation("run");
    REQUIRE(md2.animationIndex() == 1);
    md2.setAnimation(size_t{0});
    REQUIRE(md2.animationIndex() == 0);
}

TEST_CASE("md2 vertex index out of range throws", "[md2]") {
    requireMalformedMd2(kTriangleOffset, 7, "vertex index 7 out of range");
}

TEST_CASE("md2 texture coordinate index out of range throws", "[md2]") {
    requireMalformedMd2(kTriangleStOffset, 9,
                        "texture coordinate index 9 out of range");
}
