#pragma once

/*
http://tfc.duke.free.fr/coding/md2-specs-en.html
http://tfc.duke.free.fr/old/models/md2.htm
*/

#include "gl.hpp"

#include <glm/glm.hpp>

#include <boost/algorithm/clamp.hpp>

#include <array>
#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class Shader;
class PAK;

class MD2 {
public:
    static int32_t const ident = 844121161;
    static int32_t const version = 8;
    static int32_t const max_tris = 4096;
    static int32_t const max_vertices = 2048;
    static int32_t const max_texcoords = 2048;
    static int32_t const max_frames = 512;
    static int32_t const max_skins = 32;

    struct Header {
        int32_t ident;
        int32_t version;
        int32_t skinwidth;
        int32_t skinheight;
        int32_t framesize;
        int32_t num_skins;
        int32_t num_xyz;
        int32_t num_st;
        int32_t num_tris;
        int32_t num_glcmds;
        int32_t num_frames;
        int32_t offset_skins;
        int32_t offset_st;
        int32_t offset_tris;
        int32_t offset_frames;
        int32_t offset_glcmds;
        int32_t offset_end;
    };

    struct Skin {
        std::array<char, 64> name{};
    };

    struct TexCoord {
        int16_t s;
        int16_t t;
    };

    struct Triangle {
        std::array<uint16_t, 3> vertex;
        std::array<uint16_t, 3> st;
    };

    struct Vertex {
        std::array<uint8_t, 3> v;
        uint8_t normal_index;
    };

    struct Frame {
        std::array<float, 3> scale;
        std::array<float, 3> translate;
        std::array<char, 16> name;
        std::vector<Vertex> vertices;
    };

    struct Animation {
        std::string name;
        int start_frame;
        int end_frame;
        bool loop;

        Animation()
            : name()
            , start_frame(-1)
            , end_frame(-1)
            , loop(true) {}

        explicit Animation(std::string id)
            : name(std::move(id))
            , start_frame(-1)
            , end_frame(-1)
            , loop(true) {}
    };

    struct SkinData {
        std::string fpath;
        std::string name;

        SkinData(std::string fp, std::string n)
            : fpath(std::move(fp))
            , name(std::move(n)) {}
    };

    MD2() = default;
    explicit MD2(std::string const& filename, PAK const& pak);
    ~MD2();

    MD2(MD2 const&) = delete;
    MD2& operator=(MD2 const&) = delete;
    MD2(MD2&&) = delete;
    MD2& operator=(MD2&&) = delete;

    // accessors
    Header const& header() const { return hdr_; }

    void draw(Shader& shader);
    void update(float delta_time);

    std::vector<SkinData> const& skins() const { return skins_; }

    void set_animation(std::string const& id);
    void set_animation(size_t id);

    size_t animation_index() const { return current_animation_index_; }
    size_t skin_index() const { return current_skin_index_; }
    void set_skin_index(size_t idx);

    SkinData const& current_skin() const {
        assert(current_skin_index_ >= 0 &&
               static_cast<size_t>(current_skin_index_) < skins_.size());
        return skins_[current_skin_index_];
    }

    std::vector<Animation> const& animations() const { return animations_; }

    float frames_per_second() const { return frames_per_second_; }
    void set_frames_per_second(float f) {
        frames_per_second_ = boost::algorithm::clamp(f, 0.0f, 60.0f);
    }

private:
    void setup_buffers();
    [[nodiscard]] bool validate_header(Header const& hdr);
    [[nodiscard]] bool load(PAK const&, std::string const& filename);
    [[nodiscard]] bool load(std::ifstream&);
    [[nodiscard]] bool load_skins(std::ifstream&, size_t);
    [[nodiscard]] bool load_triangles(std::ifstream&, size_t);
    [[nodiscard]] bool load_texcoords(std::ifstream&, size_t);
    [[nodiscard]] bool load_frames(std::ifstream&, size_t);
    void load_skins_from_directory(std::filesystem::path const& dpath,
                                   std::filesystem::path const& root);

private:
    struct KeyFrame {
        std::vector<glm::vec3> vertices;
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

    GLuint vao_{};
    std::array<GLuint, 2> vbo_{};

    // animation state
    int32_t current_animation_index_{};
    int next_frame_{};
    int current_frame_{};
    float interpolation_{};
    float frames_per_second_ = 8.0f;

    int32_t current_skin_index_{};
};

std::ostream& operator<<(std::ostream&, MD2::Header const&);
std::ostream& operator<<(std::ostream&, MD2::Animation const&);
