#pragma once

/// @see http://tfc.duke.free.fr/coding/md2-specs-en.html
/// @see http://tfc.duke.free.fr/old/models/md2.htm

#include <boost/algorithm/clamp.hpp>
#include <glm/glm.hpp>
#include <gsl-lite/gsl-lite.hpp>

#include <array>
#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <string>
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
///   `interpolated_vertices_`.
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
    static int32_t const ident = 844121161; ///< Magic number ("IDP2").
    static int32_t const version = 8;
    static int32_t const max_tris = 4096;
    static int32_t const max_vertices = 2048;
    static int32_t const max_texcoords = 2048;
    static int32_t const max_frames = 512;
    static int32_t const max_skins = 32;
    /// @}

    /// On-disk file header (17 × int32, no padding).
    struct Header {
        int32_t ident;      ///< Must equal MD2::ident.
        int32_t version;    ///< Must equal MD2::version.
        int32_t skinwidth;  ///< Skin texture width in pixels.
        int32_t skinheight; ///< Skin texture height in pixels.
        int32_t framesize;  ///< Size of one frame in bytes.
        int32_t num_skins;  ///< Number of skin entries.
        int32_t num_xyz;    ///< Number of vertices per frame.
        int32_t num_st;     ///< Number of texture coordinate pairs.
        int32_t num_tris;   ///< Number of triangles.
        int32_t num_glcmds; ///< Number of GL commands (unused by this loader).
        int32_t num_frames; ///< Total number of keyframes.
        int32_t offset_skins;  ///< File offset to the skin section.
        int32_t offset_st;     ///< File offset to the texcoord section.
        int32_t offset_tris;   ///< File offset to the triangle section.
        int32_t offset_frames; ///< File offset to the frame section.
        int32_t offset_glcmds; ///< File offset to GL commands.
        int32_t offset_end;    ///< File size.
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
        uint8_t normal_index;
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
        int start_frame; ///< Index of the first frame (inclusive).
        int end_frame;   ///< Index of the last frame (inclusive).
        bool loop;       ///< If false, animation stops at end_frame.

        Animation()
            : start_frame(-1)
            , end_frame(-1)
            , loop(true) {}

        explicit Animation(std::string id)
            : name(std::move(id))
            , start_frame(-1)
            , end_frame(-1)
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
    explicit MD2(std::string const& filename, PAK const& pak);

    MD2(MD2 const&) = delete;
    MD2& operator=(MD2 const&) = delete;
    MD2(MD2&&) = delete;
    MD2& operator=(MD2&&) = delete;

    /// @name Accessors
    /// @{
    Header const& header() const { return hdr_; }
    std::vector<SkinData> const& skins() const { return skins_; }
    std::vector<Animation> const& animations() const { return animations_; }
    size_t animation_index() const { return current_animation_index_; }
    size_t skin_index() const { return current_skin_index_; }
    float frames_per_second() const { return frames_per_second_; }

    SkinData const& current_skin() const {
        return gsl_lite::at(skins_, current_skin_index_);
    }

    /// World-space vertex positions for the current interpolated frame.
    /// Size is `num_tris × 3` (one entry per triangle corner).
    /// This is the data that should be uploaded to the GPU via
    /// `GL::Mesh::sync()`.
    std::vector<glm::vec3> const& interpolated_vertices() const {
        return interpolated_vertices_;
    }

    /// Normalised texture coordinates, parallel to `interpolated_vertices()`.
    /// Static after construction — does not change between frames.
    std::vector<glm::vec2> const& scaled_texcoords() const {
        return scaled_texcoords_;
    }
    /// @}

    /// Advance the animation by @p dt seconds and update
    /// `interpolated_vertices()`.
    ///
    /// No-op if the animation is paused (fps == 0), is a single frame, or is a
    /// non-looping animation that has reached its last frame.
    void update(float dt);

    /// Draw an ImGui panel for selecting animations, skins, and playback speed.
    /// @return True if the active skin changed (caller should reload the
    /// texture).
    [[nodiscard]] bool draw_ui();

    /// Switch to the animation with the given name. No-op if not found.
    void set_animation(std::string const& id);

    /// Switch to the animation at @p index.
    /// @throws gsl_lite::fail_fast if @p index is out of range.
    void set_animation(size_t index);

    /// @throws gsl_lite::fail_fast if @p index is out of range.
    void set_skin_index(size_t index);

    /// @param f Frames per second, clamped to [0, 60]. Pass 0 to pause.
    void set_frames_per_second(float f) {
        frames_per_second_ = boost::algorithm::clamp(f, 0.0f, 60.0f);
    }

private:
    [[nodiscard]] static bool validate_header(Header const& hdr);
    [[nodiscard]] bool load(PAK const& pf, std::string const& filename);
    [[nodiscard]] bool load(std::ifstream& infile);
    [[nodiscard]] bool load_skins(std::ifstream& infile, size_t offset);
    [[nodiscard]] bool load_triangles(std::ifstream& infile, size_t offset);
    [[nodiscard]] bool load_texcoords(std::ifstream& infile, size_t offset);
    [[nodiscard]] bool load_frames(std::ifstream& infile, size_t offset);
    void load_skins_from_directory(std::filesystem::path const& dpath,
                                   std::filesystem::path const& root);

    /// Pre-scaled, triangle-unpacked vertex positions for one keyframe.
    struct KeyFrame {
        std::vector<glm::vec3>
            vertices; ///< num_tris × 3 world-space positions.
    };

    Header hdr_{};
    std::vector<Triangle> triangles_;
    std::vector<TexCoord> texcoords_;
    std::vector<Frame> frames_;
    std::vector<KeyFrame> key_frames_;
    std::vector<glm::vec2> scaled_texcoords_;
    std::vector<SkinData> skins_;
    std::vector<Animation> animations_;
    std::unordered_map<std::string, size_t> animation_index_map_;
    std::vector<glm::vec3> interpolated_vertices_;

    int next_frame_{};
    int current_frame_{};
    float interpolation_{};
    float frames_per_second_ = 8.0f;

    std::size_t current_animation_index_{};
    std::size_t current_skin_index_{};
};

std::ostream& operator<<(std::ostream& os, MD2::Header const& hdr);
std::ostream& operator<<(std::ostream& os, MD2::Animation const& anim);
