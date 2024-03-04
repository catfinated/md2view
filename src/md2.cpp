#include "md2.hpp"
#include "pak.hpp"
#include "shader.hpp"
#include "common.hpp"

#include <glm/gtx/compatibility.hpp>
#include <spdlog/spdlog.h>
#include <fmt/ostream.h>

#include <cctype>
#include <fstream>
#include <filesystem>
#include <string_view>
#include <vector>

template <> struct fmt::formatter<MD2::Header> : ostream_formatter {};
template <> struct fmt::formatter<MD2::Animation> : ostream_formatter {};

std::string animation_id_from_frame_name(std::string const& name)
{
    std::string id;

    for (auto& ch : name) {
        if (isdigit(ch)) {
            break;
        }

        id += ch;
    }

    return id;
}

MD2::MD2(std::string const& filename, PAK const * pak)
{
    bool status = pak ? load(*pak, filename) : load(filename);

    if (!status) {
        throw std::runtime_error("failed to load MD2 model " + filename);
    }

    set_animation(0);
}

MD2::~MD2()
{
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(2, vbo_);
}

bool MD2::load(PAK const& pf, std::string const& filename)
{
    spdlog::info("loading model {} from pak {}", filename, pf.fpath().string());
    if (filename.empty()) { return false; }
    
    auto node = pf.find(filename);
    if (!node) { 
        spdlog::error("model not found {}", filename);
        return false; 
    }

    auto ispak = !pf.isDirectory();
    auto p = ispak ? pf.fpath() : pf.fpath() / filename;
    spdlog::info("{}", p.string());

    std::ifstream inf(p, std::ios_base::binary);
    if (!inf) { return false; }

    inf.seekg(node->filepos);

    auto result = load(inf, ispak ? filename : p.string(), ispak);
    inf.close();
    return result;
}

bool MD2::load(std::string const& filename)
{
    if (filename.empty()) { return false; }

    spdlog::info("loading model from file {}", filename);

    std::ifstream inf(filename.c_str(), std::ios::binary);

    if (!inf.good()) { return false; }

    auto result = load(inf, filename);
    inf.close();
    return result;
}

bool MD2::load(std::ifstream& infile, std::string const& filename, bool ispak)
{
    next_frame_ = 1;
    current_frame_ = 0;
    current_animation_index_ = -1;
    current_skin_index_ = -1;
    interpolation_ = 0.0f;

    static_assert(sizeof(hdr_) == (17 * sizeof(int32_t)), "md2 header has padding");

    size_t offset = infile.tellg(); // header offset
    infile.read(reinterpret_cast<char * >(&hdr_), sizeof(hdr_));
    spdlog::debug("md2 header: {}", hdr_);

    MD2V_EXPECT(load_skins(infile, offset, filename, ispak));
    MD2V_EXPECT(load_triangles(infile, offset));
    MD2V_EXPECT(load_texcoords(infile, offset));
    MD2V_EXPECT(load_frames(infile, offset));

    setup_buffers();
    set_animation(0);
    set_skin_index(0);

    return true;
}

bool MD2::load_skins(std::ifstream& infile, size_t offset, std::string const& filename, bool ispak)
{
    MD2V_EXPECT(infile);

    // read skins
    static_assert(sizeof(Skin) == 64, "md2 skin has padding");
    std::vector<Skin> skins;
    skins.resize(hdr_.num_skins); // need resize so we actually push elements
    infile.seekg(offset + hdr_.offset_skins);
    infile.read(reinterpret_cast<char * >(skins.data()), sizeof(Skin) * skins.size());
    assert(skins.size() == static_cast<size_t>(hdr_.num_skins));
    spdlog::info("num skins={}", skins.size());

    auto root = std::filesystem::path(filename).parent_path();
    // todo: if !ispak just look for skin files in the same directory

    for (auto const& skin : skins) {
        spdlog::info("skin: '{}'", std::string_view{skin.name.data(), skin.name.size()});
        std::filesystem::path f(std::string(skin.name.data()));

        if (ispak) {
            skins_.emplace_back(f.string(), f.stem().string());
            spdlog::debug("{}", skins_.back().fpath);
        }
        else {
            auto p = root / f.filename();
            auto cp = p;
            p.replace_extension("png");

            if (std::filesystem::exists(cp)) {
                skins_.emplace_back(cp.string(), cp.stem().string());
                spdlog::debug("{}", skins_.back().fpath);
            }
            else if (std::filesystem::exists(p)) {
                skins_.emplace_back(p.string(), cp.stem().string());
                spdlog::debug("{}", skins_.back().fpath);
            }
        }
    }

    // for drfreak model
    if (!ispak && skins_.empty()) {
        auto base = root.filename();
        auto p =  root / base;
        skins_.emplace_back(p.string() + ".png", base.string());
        spdlog::debug("{}", skins_.back().fpath);
    }

    return infile.good();
}

bool MD2::load_triangles(std::ifstream& infile, size_t offset)
{
    MD2V_EXPECT(infile);
    static_assert(sizeof(Triangle) == 6 * sizeof(uint16_t), "md2 triangle has padding");
    triangles_.resize(hdr_.num_tris);
    infile.seekg(offset + hdr_.offset_tris);
    infile.read(reinterpret_cast<char *>(triangles_.data()), sizeof(Triangle) * triangles_.size());

    return infile.good();
}

bool MD2::load_texcoords(std::ifstream& infile, size_t offset)
{
    MD2V_EXPECT(infile);
    MD2V_EXPECT(!triangles_.empty());

    // read texcoords
    static_assert(sizeof(TexCoord) == 2 * sizeof(int16_t), "md2 texcoord has padding");
    texcoords_.resize(hdr_.num_st);
    infile.seekg(offset + hdr_.offset_st);
    infile.read(reinterpret_cast<char *>(texcoords_.data()), sizeof(TexCoord) * texcoords_.size());

    // we must scale the texcoords and unpack the triangles
    // into a flat vector to better work gith glDrawArrays
    scaled_texcoords_.reserve(hdr_.num_tris * 3);

    for (auto const& triangle : triangles_) {
        for (size_t i = 0; i < 3; ++i) {
          auto st_index = triangle.st[i];
          assert(st_index < texcoords_.size());
          auto const& st = texcoords_[st_index];
          glm::vec2 scaled;
          scaled.s = static_cast<float>(st.s) / static_cast<float>(hdr_.skinwidth);
          scaled.t = static_cast<float>(st.t) / static_cast<float>(hdr_.skinheight);
          scaled_texcoords_.push_back(scaled);
      }
    }

    return infile.good();
}

bool MD2::load_frames(std::ifstream& infile, size_t offset)
{
    MD2V_EXPECT(infile);

    frames_.resize(hdr_.num_frames);
    key_frames_.resize(hdr_.num_frames);
    infile.seekg(offset + hdr_.offset_frames);

    Animation current_anim;
    current_anim.start_frame = -1;

    for (auto i = 0; i < hdr_.num_frames; ++i) {
        auto& frame = frames_[i];

        frame.vertices.resize(hdr_.num_xyz); // same # of vertices for each keyframe

        infile.read(reinterpret_cast<char *>(frame.scale.data()), sizeof(frame.scale));
        infile.read(reinterpret_cast<char *>(frame.translate.data()), sizeof(frame.translate));
        infile.read(reinterpret_cast<char *>(frame.name.data()), sizeof(frame.name));
        infile.read(reinterpret_cast<char *>(frame.vertices.data()), sizeof(Vertex) * hdr_.num_xyz);

        std::string anim_id = animation_id_from_frame_name(std::string(frame.name.data()));

        if (current_anim.name == anim_id) {
            current_anim.end_frame = i;
        }
        else {
            if (current_anim.start_frame != -1) {
                animation_index_map_[current_anim.name] = animations_.size();
                animations_.push_back(current_anim);
            }

            current_anim.start_frame = i;
            current_anim.end_frame = i;
            current_anim.name = anim_id;
        }

        // our key frame contains the scaled vertices
        // it also unpacks all the vertices from the triangle
        // into a flat vector. this is because md2 allows
        // the same vertex to have a different tex coord in two
        // different triangles which makes it difficult to use with
        // glDrawElements / glDrawArrays. so our KeyFrame
        // has num_tris * 3 entries and our tex coord
        // buffer ends up with num_tris * 3 entires but
        // that data is shared by all frames
        auto& key_frame = key_frames_[i];
        key_frame.vertices.reserve(hdr_.num_tris * 3);

        for (auto const& triangle : triangles_) {
            for (size_t i = 0; i < 3; ++i) {
                auto vertex_index = triangle.vertex[i];
                assert(vertex_index < frame.vertices.size());
                auto const& vertex = frame.vertices[vertex_index];
                glm::vec3 scaled;
                scaled.x = (frame.scale[0] * vertex.v[0]) + frame.translate[0];
                scaled.z = (frame.scale[1] * vertex.v[1]) + frame.translate[1];
                scaled.y = (frame.scale[2] * vertex.v[2]) + frame.translate[2];
                key_frame.vertices.push_back(scaled);
            }
        }
        assert(key_frame.vertices.size() == static_cast<size_t>(hdr_.num_tris * 3));
    }

    assert(key_frames_.size() == frames_.size());
    assert(frames_.size() == static_cast<size_t>(hdr_.num_frames));

    if (current_anim.start_frame != -1) {
        animation_index_map_[current_anim.name] = animations_.size();
        animations_.push_back(current_anim);
    }

    for (auto const& anim : animations_) {
        spdlog::debug("animation: {}", anim);
    }

    interpolated_vertices_ = key_frames_[0].vertices;

    return infile.good();
}

void MD2::setup_buffers()
{
   glGenVertexArrays(1, &vao_);
   glGenBuffers(2, vbo_);

   glBindVertexArray(vao_);
   spdlog::debug("vertex buffers: {} {}", vbo_[0], vbo_[1]);

   auto const& vertices = interpolated_vertices_;

   spdlog::info("num xyz: {}", vertices.size());
   static_assert(sizeof(glm::vec3) == 12, "bad vec3 size");

   glBindBuffer(GL_ARRAY_BUFFER, vbo_[0]);
   glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_DYNAMIC_DRAW);

   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

   static_assert(sizeof(glm::vec2) == 8, "bad vec2 size");

   glBindBuffer(GL_ARRAY_BUFFER, vbo_[1]);
   glBufferData(GL_ARRAY_BUFFER, scaled_texcoords_.size() * sizeof(glm::vec2), &scaled_texcoords_[0], GL_STATIC_DRAW);

   glEnableVertexAttribArray(1);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

   glBindVertexArray(0);

   glCheckError();
}

void MD2::set_animation(std::string const& id)
{
    auto iter = animation_index_map_.find(id);

    if (iter != animation_index_map_.end()) {
        auto index = iter->second;
        set_animation(index);
    }
}

void MD2::set_animation(size_t index)
{
    assert(index < animations_.size());

    if (current_animation_index_ != static_cast<int32_t>(index)) {
        auto const& anim = animations_[index];
        next_frame_ = anim.start_frame;
        current_animation_index_ = index;
        interpolation_ = 0.0;
    }
}

void MD2::set_skin_index(size_t index)
{
    assert(index < skins_.size());

    current_skin_index_ = index;
}

void MD2::update(float dt)
{
    if (current_animation_index_ < 0) {
        return;
    }

    if (frames_per_second_ == 0.0f) {
        return;
    }

    assert(static_cast<size_t>(current_animation_index_) < animations_.size());

    auto const& anim = animations_[current_animation_index_];

    interpolation_ += dt * frames_per_second_;

    if (interpolation_ >= 1.0f) {
        current_frame_ = next_frame_;
        ++next_frame_;

        if (next_frame_ > anim.end_frame) {
            if (anim.loop) {
                next_frame_ = anim.start_frame;
            }
            else {
                next_frame_  = anim.end_frame;
                current_frame_ = anim.end_frame;
            }
        }

        interpolation_ = 0.0f;
    }

    float t = interpolation_;
    int i = 0;

    if (next_frame_ == current_frame_) { return; }

    for (auto& vertex : interpolated_vertices_) {
        auto const& v1 = key_frames_[current_frame_].vertices[i];
        auto const& v2 = key_frames_[next_frame_].vertices[i];
        vertex = lerp(v1, v2, glm::vec3(t, t, t));
        ++i;
   }

   glBindBuffer(GL_ARRAY_BUFFER, vbo_[0]);
   glBufferSubData(GL_ARRAY_BUFFER, 0,
                   sizeof(glm::vec3) * interpolated_vertices_.size(),
                   &interpolated_vertices_[0]);
}

void MD2::draw(Shader& shader)
{
   glBindVertexArray(vao_);
   glDrawArrays(GL_TRIANGLES, 0, interpolated_vertices_.size());
   glCheckError();
}

bool MD2::validate_header(Header const& hdr)
{
    if (hdr.ident != MD2::version) {
      return false;
    }

    if (hdr.version != MD2::ident) {
        return false;
    }

    if (hdr.num_tris > MD2::max_tris) {
        return false;
    }

    if (hdr.num_xyz > MD2::max_vertices) {
        return false;
    }

    if (hdr.num_st > MD2::max_texcoords) {
        return false;
    }

    if (hdr.num_frames > MD2::max_frames) {
        return false;
    }

    if (hdr.num_skins > MD2::max_skins) {
        return false;
    }

    return true;
}

std::ostream& operator<<(std::ostream& os, MD2::Animation const& anim)
{
    os << '\n'
       << "id:    " << anim.name        << '\n'
       << "start: " << anim.start_frame << '\n'
       << "end:   " << anim.end_frame   << '\n'
       << "loop:  " << std::boolalpha   << anim.loop << '\n';

    return os;
}

std::ostream& operator<<(std::ostream& os, MD2::Header const& hdr)
{
    os << '\n'
       << "ident:         " << hdr.ident         << '\n'
       << "version:       " << hdr.version       << '\n'
       << "skinwidth:     " << hdr.skinwidth     << '\n'
       << "skinheight:    " << hdr.skinheight    << '\n'
       << "framesize:     " << hdr.framesize     << '\n'
       << "num_skins:     " << hdr.num_skins     << '\n'
       << "num_xyz:       " << hdr.num_xyz       << '\n'
       << "num_st:        " << hdr.num_st        << '\n'
       << "num_tris:      " << hdr.num_tris      << '\n'
       << "num_glcmds:    " << hdr.num_glcmds    << '\n'
       << "num_frames:    " << hdr.num_frames    << '\n'
       << "offset_skins:  " << hdr.offset_skins  << '\n'
       << "offset_st:     " << hdr.offset_st     << '\n'
       << "offset_tris:   " << hdr.offset_tris   << '\n'
       << "offset_frames: " << hdr.offset_frames << '\n'
       << "offset_glcmds: " << hdr.offset_glcmds << '\n'
       << "offset_end:    " << hdr.offset_end;

    return os;
}

