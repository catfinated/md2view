#pragma once

/// @see http://tfc.duke.free.fr/coding/md2-specs-en.html
/// @see http://tfc.duke.free.fr/old/models/md2.htm

#include <boost/algorithm/clamp.hpp>
#include <glm/glm.hpp>
#include <gsl-lite/gsl-lite.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

class PAK;

/// Quake II MD2 keyframe model: parser, animation state machine, and
/// interpolator.
///
/// MD2 is a keyframe animation format. Each frame stores compressed vertex
/// positions (3 × uint8 scaled by per-frame floats). This class:
/// - Parses all sections of the binary file (header, skins, texcoords,
///   triangles, frames).
/// - Unpacks triangles into a flat vertex buffer suitable for `glDrawArrays`
///   (one entry per triangle corner, not per unique vertex).
/// - Runs a frame interpolation state machine; `update(dt)` advances the
///   animation and writes lerped world-space positions into
///   `interpolatedVertices_`.
///
/// **This class has no OpenGL dependency.** GPU upload is handled separately
/// by `GL::Mesh`. Because no graphics context is required, MD2 objects can be
/// constructed and tested without a display.
///
/// Coordinate remapping: the loader swaps Y and Z to convert from Quake's
/// right-handed Z-up convention to OpenGL's Y-up convention
/// (`v[0]`→x, `v[1]`→z, `v[2]`→y).
class MD2 {
public:
    /// @name File format constants
    /// @{
    static int32_t const kIdent = 844121161; ///< Magic number ("IDP2").
    static int32_t const kVersion = 8;
    static int32_t const kMaxTris = 4096;
    static int32_t const kMaxVertices = 2048;
    static int32_t const kMaxTexcoords = 2048;
    static int32_t const kMaxFrames = 512;
    static int32_t const kMaxSkins = 32;
    /// @}

    /// On-disk file header (17 × int32, no padding).
    struct Header {
        int32_t ident;       ///< Must equal MD2::kIdent.
        int32_t version;     ///< Must equal MD2::kVersion.
        int32_t skinwidth;   ///< Skin texture width in pixels.
        int32_t skinheight;  ///< Skin texture height in pixels.
        int32_t framesize;   ///< Size of one frame in bytes.
        int32_t numSkins;    ///< Number of skin entries.
        int32_t numXyz;      ///< Number of vertices per frame.
        int32_t numSt;       ///< Number of texture coordinate pairs.
        int32_t numTris;     ///< Number of triangles.
        int32_t numGlcmds;   ///< Number of GL commands (unused by this loader).
        int32_t numFrames;   ///< Total number of keyframes.
        int32_t offsetSkins; ///< File offset to the skin section.
        int32_t offsetSt;    ///< File offset to the texcoord section.
        int32_t offsetTris;  ///< File offset to the triangle section.
        int32_t offsetFrames; ///< File offset to the frame section.
        int32_t offsetGlcmds; ///< File offset to GL commands.
        int32_t offsetEnd;    ///< File size.
    };

    /// A skin name entry (64-byte null-padded string).
    struct Skin {
        std::array<char, 64> name{};
    };

    /// Raw integer texture coordinate pair as stored on disk.
    /// Scale by (1/skinwidth, 1/skinheight) to get normalised UVs.
    struct TexCoord {
        int16_t s;
        int16_t t;
    };

    /// A triangle referencing three vertex indices and three texcoord indices.
    struct Triangle {
        std::array<uint16_t, 3>
            vertex;                 ///< Indices into the frame vertex array.
        std::array<uint16_t, 3> st; ///< Indices into the texcoord array.
    };

    /// A compressed vertex: three uint8 components scaled by the frame's
    /// scale/translate vectors, plus a pre-computed normal index.
    struct Vertex {
        std::array<uint8_t, 3> v;
        uint8_t normalIndex;
    };

    /// One keyframe: per-frame scale/translate, a name, and the raw vertices.
    struct Frame {
        std::array<float, 3> scale{};
        std::array<float, 3> translate{};
        std::array<char, 16> name{};
        std::vector<Vertex> vertices;
    };

    /// A named animation consisting of a contiguous range of frames.
    ///
    /// Animation names are derived by stripping trailing digits from frame
    /// names (e.g. `"stand0"`, `"stand1"` → animation `"stand"`).
    struct Animation {
        std::string name;
        int startFrame; ///< Index of the first frame (inclusive).
        int endFrame;   ///< Index of the last frame (inclusive).
        bool loop;      ///< If false, animation stops at endFrame.

        Animation()
            : startFrame(-1)
            , endFrame(-1)
            , loop(true) {}

        explicit Animation(std::string id)
            : name(std::move(id))
            , startFrame(-1)
            , endFrame(-1)
            , loop(true) {}
    };

    /// Resolved skin entry pairing a PAK-relative file path with a display
    /// name.
    struct SkinData {
        std::string fpath; ///< PAK-relative path to the skin image.
        std::string name;  ///< Display name (file stem).

        SkinData(std::string fp, std::string n)
            : fpath(std::move(fp))
            , name(std::move(n)) {}
    };

    /// Load an MD2 model from a named entry within a PAK.
    ///
    /// @param filename Archive-relative path (e.g. `"models/player/tris.md2"`).
    /// @param pak      The archive or directory to load from.
    /// @throws std::runtime_error if the file cannot be opened or parsed.
    explicit MD2(std::string_view filename, PAK const& pak);
    ~MD2() = default;

    MD2(MD2 const&) = delete;
    MD2& operator=(MD2 const&) = delete;
    MD2(MD2&&) = delete;
    MD2& operator=(MD2&&) = delete;

    /// @name Accessors
    /// @{
    [[nodiscard]] Header const& header() const { return hdr_; }
    [[nodiscard]] std::vector<SkinData> const& skins() const { return skins_; }
    [[nodiscard]] std::vector<Animation> const& animations() const {
        return animations_;
    }
    [[nodiscard]] size_t animationIndex() const {
        return currentAnimationIndex_;
    }
    [[nodiscard]] size_t skinIndex() const { return currentSkinIndex_; }
    [[nodiscard]] float framesPerSecond() const { return framesPerSecond_; }

    [[nodiscard]] SkinData const& currentSkin() const {
        return gsl_lite::at(skins_, currentSkinIndex_);
    }

    /// World-space vertex positions for the current interpolated frame.
    /// Size is `numTris × 3` (one entry per triangle corner).
    /// This is the data that should be uploaded to the GPU via
    /// `GL::Mesh::sync()`.
    [[nodiscard]] std::vector<glm::vec3> const& interpolatedVertices() const {
        return interpolatedVertices_;
    }

    /// Normalised texture coordinates, parallel to `interpolatedVertices()`.
    /// Static after construction — does not change between frames.
    [[nodiscard]] std::vector<glm::vec2> const& scaledTexcoords() const {
        return scaledTexcoords_;
    }
    /// @}

    /// Advance the animation by @p dt seconds and update
    /// `interpolatedVertices()`.
    ///
    /// No-op if the animation is paused (fps == 0), is a single frame, or is a
    /// non-looping animation that has reached its last frame.
    void update(float dt);

    /// Switch to the animation with the given name. No-op if not found.
    void setAnimation(std::string const& id);

    /// Switch to the animation at @p index.
    /// @throws gsl_lite::fail_fast if @p index is out of range.
    void setAnimation(size_t index);

    /// @throws gsl_lite::fail_fast if @p index is out of range.
    void setSkinIndex(size_t index);

    /// @param f Frames per second, clamped to [0, 60]. Pass 0 to pause.
    void setFramesPerSecond(float f) {
        framesPerSecond_ = boost::algorithm::clamp(f, 0.0f, 60.0f);
    }

private:
    [[nodiscard]] bool load(PAK const& pf, std::string_view filename);
    [[nodiscard]] bool load(std::ifstream& infile);
    [[nodiscard]] bool loadSkins(std::ifstream& infile, size_t offset);
    [[nodiscard]] bool loadTriangles(std::ifstream& infile, size_t offset);
    [[nodiscard]] bool loadTexcoords(std::ifstream& infile, size_t offset);
    [[nodiscard]] bool loadFrames(std::ifstream& infile, size_t offset);
    void loadSkinsFromDirectory(std::filesystem::path const& dpath,
                                std::filesystem::path const& root);

    /// Pre-scaled, triangle-unpacked vertex positions for one keyframe.
    struct KeyFrame {
        std::vector<glm::vec3> vertices; ///< numTris × 3 world-space positions.
    };

    /// Scale a frame's packed vertices and unpack them in triangle order.
    ///
    /// @throws std::runtime_error if a triangle references a vertex the frame
    /// does not have.
    [[nodiscard]] KeyFrame unpackFrame(Frame const& frame) const;

    Header hdr_{};
    std::vector<Triangle> triangles_;
    std::vector<TexCoord> texcoords_;
    std::vector<Frame> frames_;
    std::vector<KeyFrame> keyFrames_;
    std::vector<glm::vec2> scaledTexcoords_;
    std::vector<SkinData> skins_;
    std::vector<Animation> animations_;
    std::unordered_map<std::string, size_t> animationIndexMap_;
    std::vector<glm::vec3> interpolatedVertices_;

    int nextFrame_{};
    int currentFrame_{};
    float interpolation_{};
    float framesPerSecond_ = 8.0f;

    std::size_t currentAnimationIndex_{};
    std::size_t currentSkinIndex_{};
};

std::ostream& operator<<(std::ostream& os, MD2::Header const& hdr);
std::ostream& operator<<(std::ostream& os, MD2::Animation const& anim);
