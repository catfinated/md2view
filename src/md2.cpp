#include "md2.hpp"
#include "common.hpp"

#include <boost/filesystem.hpp>

#include <fstream>
#include <iostream>
#include <vector>
#include <cctype>

namespace blue { namespace md2 {

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

bool Model::load(std::string const& filename)
{
    next_frame_ = 1;
    current_frame_ = 0;
    current_animation_index_ = -1;
    current_skin_index_ = -1;
    interpolation_ = 0.0f;

    if (filename.empty()) {
        return false;
    }

    std::cout << filename << '\n';

    std::ifstream infile(filename.c_str(),
                         std::ios::binary);

    if (!infile.good()) {
        return false;
    }

    static_assert(sizeof(hdr_) == (17 * sizeof(int32_t)), "md2 header has padding");

    infile.read(reinterpret_cast<char * >(&hdr_), sizeof(hdr_));

    std::cout << hdr_ << '\n';

    // read skins
    static_assert(sizeof(Skin) == 64, "md2 skin has padding");
    std::vector<Skin> skins;
    skins.resize(hdr_.num_skins); // need resize so we actually push elements
    infile.seekg(hdr_.offset_skins);
    infile.read(reinterpret_cast<char * >(skins.data()),
                sizeof(Skin) * skins.size());
    assert(skins.size() == static_cast<size_t>(hdr_.num_skins));
    std::cout << "num skins=" << skins.size() << '\n';

    auto root = boost::filesystem::path(filename).parent_path();

    for (auto const& skin : skins) {
        //std::cout << skin.name << '\n';
        boost::filesystem::path f(std::string(skin.name));
        auto p = root / f.filename();
        //auto p = root / "Ogrobase_orig.tga";

        auto cp = p;
        p.replace_extension("png");
        if (boost::filesystem::exists(cp)) {
            skins_.emplace_back(cp.string(), cp.stem().string());
            std::cout << skins_.back().fpath << '\n';
        }
        else if (boost::filesystem::exists(p)) {
            skins_.emplace_back(p.string(), cp.stem().string());
            std::cout << skins_.back().fpath << '\n';
        }
    }

    if (skins_.empty()) {
        auto base = root.filename();
        auto p =  root / base;
        skins_.emplace_back(p.string() + ".png", base.string());
        std::cout << skins_.back().fpath << '\n';
    }

    // read triangles
    static_assert(sizeof(Triangle) == 6 * sizeof(uint16_t), "md2 triangle has padding");
    triangles_.resize(hdr_.num_tris);
    infile.seekg(hdr_.offset_tris);
    infile.read(reinterpret_cast<char *>(triangles_.data()),
                sizeof(Triangle) * triangles_.size());
    assert(triangles_.size() == static_cast<size_t>(hdr_.num_tris));

    // read texcoords
    static_assert(sizeof(TexCoord) == 2 * sizeof(int16_t), "md2 texcoord has padding");
    texcoords_.resize(hdr_.num_st);
    infile.seekg(hdr_.offset_st);
    infile.read(reinterpret_cast<char *>(texcoords_.data()),
                sizeof(TexCoord) * texcoords_.size());
    assert(texcoords_.size() == static_cast<size_t>(hdr_.num_st));

    // we must scale the texcoords and unpack the triangles
    // into a flat vector to better work gith glDrawArrays
    internal_texcoords_.reserve(hdr_.num_tris * 3);

    for (auto const& triangle : triangles_) {
        for (size_t i = 0; i < 3; ++i) {
          auto st_index = triangle.st[i];
          assert(st_index < texcoords_.size());
          auto const& st = texcoords_[st_index];
          glm::vec2 scaled;
          scaled.s = static_cast<float>(st.s) / static_cast<float>(hdr_.skinwidth);
          scaled.t = static_cast<float>(st.t) / static_cast<float>(hdr_.skinheight);
          //std::cout << scaled.s << ' ' << scaled.t << '\n';
          internal_texcoords_.push_back(scaled);
      }
    }
    assert(internal_texcoords_.size() == static_cast<size_t>(hdr_.num_tris * 3));
    //std::cout << "num st: " << internal_texcoords_.size() << '\n';

    // read frames
    frames_.resize(hdr_.num_frames);
    internal_frames_.resize(hdr_.num_frames);
    infile.seekg(hdr_.offset_frames);

    Animation current_anim;
    current_anim.start_frame = -1;

    for (auto i = 0; i < hdr_.num_frames; ++i) {
        auto& frame = frames_[i];

        frame.vertices.resize(hdr_.num_xyz); // same # of vertices for each keyframe

        infile.read(reinterpret_cast<char *>(frame.scale), sizeof(frame.scale));
        infile.read(reinterpret_cast<char *>(frame.translate), sizeof(frame.translate));
        infile.read(reinterpret_cast<char *>(frame.name), sizeof(frame.name));
        infile.read(reinterpret_cast<char *>(frame.vertices.data()), sizeof(Vertex) * hdr_.num_xyz);

        std::string anim_id = animation_id_from_frame_name(frame.name);

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

        // our 'internal frame' contains the scaled vertices
        // it also unpacks all the vertices from the triangle
        // into a flat vector. this is because md2 allows
        // the same vertex to have a different tex coord in two
        // different triangles which makes it difficult to use with
        // glDrawElements / glDrawArrays. so our InternalFrame
        // has num_tris * 3 entries and our tex coord
        // buffer ends up with num_tris * 3 entires but
        // that data is shared by all frames
        auto& internal_frame = internal_frames_[i];
        internal_frame.vertices.reserve(hdr_.num_tris * 3);

        for (auto const& triangle : triangles_) {
            for (size_t i = 0; i < 3; ++i) {
                auto vertex_index = triangle.vertex[i];
                assert(vertex_index < frame.vertices.size());
                auto const& vertex = frame.vertices[vertex_index];
                glm::vec3 scaled;
                scaled.x = (frame.scale[0] * vertex.v[0]) + frame.translate[0];
                scaled.z = (frame.scale[1] * vertex.v[1]) + frame.translate[1];
                scaled.y = (frame.scale[2] * vertex.v[2]) + frame.translate[2];
                //scaled /= 64.0;
                internal_frame.vertices.push_back(scaled);
            }
        }
        assert(internal_frame.vertices.size() == static_cast<size_t>(hdr_.num_tris * 3));

        //std::cout << "Frame; " << i << " name: " << frame.name << '\n';
    }

    assert(internal_frames_.size() == frames_.size());
    assert(frames_.size() == static_cast<size_t>(hdr_.num_frames));

    if (current_anim.start_frame != -1) {
        animation_index_map_[current_anim.name] = animations_.size();
        animations_.push_back(current_anim);
    }

    //std::cout << "num xyz: " << internal_frames_[0].vertices.size() << '\n';
    //for (auto const& v : internal_frames_[0].vertices) {
    //    std::cout << v.x << ' ' << v.y << ' ' << v.z << '\n';
    //}

    for (auto const& anim : animations_) {
        std::cout << anim << '\n';
    }

    interpolated_vertices_ = internal_frames_[0].vertices;

    setup_buffers();

    set_animation(0);
    set_skin_index(0);

    return true;
}

void Model::setup_buffers()
{
   glGenVertexArrays(1, &vao_);
   glGenBuffers(2, vbo_);

   glBindVertexArray(vao_);
   std::cout << vbo_[0] << ' ' << vbo_[1] << '\n';

   auto const& vertices = interpolated_vertices_;

   std::cout << "num xyz: " << vertices.size() << '\n';
   static_assert(sizeof(glm::vec3) == 12, "bad vec3 size");

   glBindBuffer(GL_ARRAY_BUFFER, vbo_[0]);
   glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_DYNAMIC_DRAW);

   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

   static_assert(sizeof(glm::vec2) == 8, "bad vec2 size");

   glBindBuffer(GL_ARRAY_BUFFER, vbo_[1]);
   glBufferData(GL_ARRAY_BUFFER, internal_texcoords_.size() * sizeof(glm::vec2), &internal_texcoords_[0], GL_STATIC_DRAW);

   glEnableVertexAttribArray(1);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

   glBindVertexArray(0);

   glCheckError();
}

void Model::set_animation(std::string const& id)
{
    auto iter = animation_index_map_.find(id);

    if (iter != animation_index_map_.end()) {
        auto index = iter->second;
        set_animation(index);
    }
}

void Model::set_animation(size_t index)
{
    assert(index < animations_.size());

    if (current_animation_index_ != index) {
        auto const& anim = animations_[index];
        next_frame_ = anim.start_frame;
        current_animation_index_ = index;
    }
}

void Model::set_skin_index(size_t index)
{
    assert(index < skins_.size());

    current_skin_index_ = index;
}

void Model::update(float dt)
{
    if (current_animation_index_ < 0) {
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
            } else {
                next_frame_  = anim.end_frame;
                current_frame_ = anim.end_frame;
            }
        }

        interpolation_ = 0.0f;
    }

    float t = interpolation_;
    int i = 0;

    for (auto& vertex : interpolated_vertices_) {
        float x1 = internal_frames_[current_frame_].vertices[i].x;
        float x2 = internal_frames_[next_frame_].vertices[i].x;
        vertex.x = x1 + t * (x2 - x1);

        float y1 = internal_frames_[current_frame_].vertices[i].y;
        float y2 = internal_frames_[next_frame_].vertices[i].y;
        vertex.y = y1 + t * (y2 - y1);

        float z1 = internal_frames_[current_frame_].vertices[i].z;
        float z2 = internal_frames_[next_frame_].vertices[i].z;
        vertex.z = z1 + t * (z2 - z1);

        ++i;
   }

   glBindBuffer(GL_ARRAY_BUFFER, vbo_[0]);
   glBufferSubData(GL_ARRAY_BUFFER, 0,
                   sizeof(glm::vec3) * interpolated_vertices_.size(),
                   &interpolated_vertices_[0]);
}

void Model::draw(blue::Shader& shader)
{
   glBindVertexArray(vao_);
   glDrawArrays(GL_TRIANGLES, 0, interpolated_vertices_.size());
   glCheckError();
}

bool Model::validate_header(Header const& hdr)
{
    if (hdr.ident != Model::version) {
      return false;
    }

    if (hdr.version != Model::ident) {
        return false;
    }

    if (hdr.num_tris > Model::max_tris) {
        return false;
    }

    if (hdr.num_xyz > Model::max_vertices) {
        return false;
    }

    if (hdr.num_st > Model::max_texcoords) {
        return false;
    }

    if (hdr.num_frames > Model::max_frames) {
        return false;
    }

    if (hdr.num_skins > Model::max_skins) {
        return false;
    }

    return true;
}

std::ostream& operator<<(std::ostream& os, Animation const& anim)
{
    os << "id:    " << anim.name        << '\n'
       << "start: " << anim.start_frame << '\n'
       << "end:   " << anim.end_frame   << '\n'
       << "loop:  " << std::boolalpha   << anim.loop << '\n';

    return os;
}

std::ostream& operator<<(std::ostream& os, Model::Header const& hdr)
{
    os << "ident:         " << hdr.ident         << '\n'
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

}} // namespace blue::md2
