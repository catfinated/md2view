#pragma once

/*
http://tfc.duke.free.fr/coding/md2-specs-en.html
http://tfc.duke.free.fr/old/models/md2.htm
*/

#include "shader.hpp"

#include <glm/glm.hpp>

#include <boost/algorithm/clamp.hpp>

#include <cstdint>
#include <string>
#include <iosfwd>
#include <vector>
#include <unordered_map>

namespace blue { namespace md2 {

struct Animation
{
    std::string name;
    int start_frame;
    int end_frame;
    bool loop;

    Animation()
        : name()
        , start_frame(-1)
        , end_frame(-1)
        , loop(true)
    {}

    explicit Animation(std::string const& id)
        : name(id)
        , start_frame(-1)
        , end_frame(-1)
        , loop(true)
    {}
};

class Model
{
public:
    static int32_t const ident         = 844121161;
    static int32_t const version       = 8;
    static int32_t const max_tris      = 4096;
    static int32_t const max_vertices  = 2048;
    static int32_t const max_texcoords = 2048;
    static int32_t const max_frames    = 512;
    static int32_t const max_skins     = 32;

    struct Header
    {
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

    struct Skin
    {
        char name[64];
    };

    struct TexCoord
    {
        int16_t s;
        int16_t t;
    };

    struct Triangle
    {
        uint16_t vertex[3];
        uint16_t st[3];
    };

    struct Vertex
    {
        uint8_t v[3];
        uint8_t normal_index;
    };

    struct Frame
    {
        float scale[3];
        float translate[3];
        char  name[16];
        std::vector<Vertex> vertices;
    };

    // accessors
    Header const& header() const { return hdr_; }

    // modifiers
    bool load(std::string const& filename);

    void draw(blue::Shader& shader);
    void update(float delta_time);

    std::vector<std::string> const& skins() const { return skins_; }

    void set_animation(std::string const& id);
    void set_animation(size_t id);

    size_t animation_index() const { return current_animation_index_; }

    std::vector<Animation> const& animations() const { return animations_; }

    float frames_per_second() const { return frames_per_second_; }
    void set_frames_per_second(float f) { frames_per_second_ = boost::algorithm::clamp(f, 1.0f, 60.0f); }

private:
    bool validate_header(Header const& hdr);
    void setup_buffers();

    struct InternalFrame
    {
        std::vector<glm::vec3> vertices;
    };

private:
    Header hdr_;
    std::vector<Triangle> triangles_;
    std::vector<TexCoord> texcoords_;
    std::vector<Frame>    frames_;
    std::vector<InternalFrame> internal_frames_;
    std::vector<glm::vec2> internal_texcoords_;
    std::vector<std::string> skins_;
    std::vector<Animation> animations_;
    std::unordered_map<std::string, size_t> animation_index_map_;
    std::vector<glm::vec3> interpolated_vertices_;

    GLuint vao_;
    GLuint vbo_[2];

    // animation state
    ssize_t current_animation_index_;
    int next_frame_;
    int current_frame_;
    float interpolation_;
    float frames_per_second_ = 8.0f;
};

std::ostream& operator<<(std::ostream&, Model::Header const&);
std::ostream& operator<<(std::ostream&, Animation const&);

std::string path_for_skin(Model::Skin const& skin);

}} // namespace blue::md2
