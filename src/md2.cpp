#include "md2view/md2.hpp"
#include "md2view/pak.hpp"
#include "md2view/shader.hpp"

#include <fmt/ostream.h>
#include <glm/gtx/compatibility.hpp>
#include <gsl-lite/gsl-lite.hpp>
#include <imgui.h>
#include <spdlog/spdlog.h>

#include <cctype>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <utility>
#include <vector>

template <> struct fmt::formatter<MD2::Header> : ostream_formatter {};
template <> struct fmt::formatter<MD2::Animation> : ostream_formatter {};

std::string animation_id_from_frame_name(std::string const& name) {
    std::string id;

    for (auto const& ch : name) {
        if (isdigit(ch) != 0) {
            break;
        }

        id += ch;
    }

    return id;
}

MD2::MD2(std::string const& filename, PAK const& pak) {
    if (!load(pak, filename)) {
        throw std::runtime_error("failed to load MD2 model " + filename);
    }
}

MD2::~MD2() {
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(2, vbo_.data());
}

bool MD2::load(PAK const& pf, std::string const& filename) {
    spdlog::info("loading model {} from pak {}", filename, pf.fpath().string());
    gsl_Expects(!filename.empty());
    auto ispak = !pf.is_directory();

    {
        auto inf = pf.open_ifstream(filename);
        gsl_Expects(inf);
        if (!load(inf)) {
            return false;
        }
    }

    if (!ispak) {
        auto p = pf.fpath() / filename;
        load_skins_from_directory(p.parent_path(), pf.fpath());
    }
    return true;
}

void MD2::load_skins_from_directory(std::filesystem::path const& dpath,
                                    std::filesystem::path const& root) {
    static constexpr std::array extensions = {".pcx", ".png", ".jpg"};

    spdlog::info("load skins from {}", dpath.string());
    std::vector<SkinData> found_skins;
    for (auto const& skin : skins_) {
        auto path = dpath / skin.name;
        for (auto const& ext : extensions) {
            path = path.replace_extension(ext);
            if (std::filesystem::exists(path)) {
                found_skins.emplace_back(path.lexically_relative(root).string(),
                                         path.stem().string());
                break;
            }
        }
    }

    if (found_skins.empty()) { // drfreak model has no skins specified
        for (auto const& dir_entry :
             std::filesystem::directory_iterator{dpath}) {
            if (std::filesystem::is_regular_file(dir_entry.path())) {
                auto const extension = dir_entry.path().extension();
                if (extension == ".png") {
                    auto relpath = dir_entry.path().lexically_relative(root);
                    found_skins.emplace_back(relpath.string(),
                                             relpath.stem().string());
                }
            }
        }
    }
    skins_ = std::move(found_skins);
}

bool MD2::load(std::ifstream& infile) {
    next_frame_ = 1;
    current_frame_ = 0;
    interpolation_ = 0.0f;

    static_assert(sizeof(hdr_) == (17 * sizeof(int32_t)),
                  "md2 header has padding");

    size_t offset = infile.tellg(); // header offset
    infile.read(reinterpret_cast<char*>(&hdr_), sizeof(hdr_));
    spdlog::debug("md2 header: {}", hdr_);

    gsl_Assert(load_skins(infile, offset));
    gsl_Assert(load_triangles(infile, offset));
    gsl_Assert(load_texcoords(infile, offset));
    gsl_Assert(load_frames(infile, offset));

    setup_buffers();
    return true;
}

bool MD2::load_skins(std::ifstream& infile, size_t offset) {
    gsl_Expects(infile);

    // read skins
    static_assert(sizeof(Skin) == 64, "md2 skin has padding");
    std::vector<Skin> skins;
    skins.resize(hdr_.num_skins); // need resize so we actually push elements
    infile.seekg(gsl_lite::narrow<std::streamoff>(offset + hdr_.offset_skins));
    infile.read(reinterpret_cast<char*>(skins.data()),
                gsl_lite::narrow<std::streamsize>(sizeof(Skin) * skins.size()));
    assert(static_cast<size_t>(infile.gcount()) == sizeof(Skin) * skins.size());
    assert(skins.size() == static_cast<size_t>(hdr_.num_skins));
    spdlog::info("num skins={}", skins.size());

    for (auto const& skin : skins) {
        spdlog::info("skin: '{}'",
                     std::string_view{skin.name.data(), skin.name.size()});
        std::filesystem::path f(std::string(skin.name.data()));
        skins_.emplace_back(f.string(), f.stem().string());
        spdlog::debug("{}", skins_.back().fpath);
    }

    return infile.good();
}

bool MD2::load_triangles(std::ifstream& infile, size_t offset) {
    gsl_Expects(infile);
    static_assert(sizeof(Triangle) == 6 * sizeof(uint16_t),
                  "md2 triangle has padding");
    triangles_.resize(hdr_.num_tris);
    infile.seekg(gsl_lite::narrow<std::streamoff>(offset + hdr_.offset_tris));
    infile.read(reinterpret_cast<char*>(triangles_.data()),
                gsl_lite::narrow<std::streamsize>(sizeof(Triangle) *
                                                  triangles_.size()));

    return infile.good();
}

bool MD2::load_texcoords(std::ifstream& infile, size_t offset) {
    gsl_Expects(infile);
    gsl_Expects(!triangles_.empty());

    // read texcoords
    static_assert(sizeof(TexCoord) == 2 * sizeof(int16_t),
                  "md2 texcoord has padding");
    texcoords_.resize(hdr_.num_st);
    infile.seekg(gsl_lite::narrow<std::streamoff>((offset + hdr_.offset_st)));
    infile.read(reinterpret_cast<char*>(texcoords_.data()),
                gsl_lite::narrow<std::streamsize>(sizeof(TexCoord) *
                                                  texcoords_.size()));

    // we must scale the texcoords and unpack the triangles
    // into a flat vector to better work gith glDrawArrays
    scaled_texcoords_.reserve(hdr_.num_tris * 3);

    for (auto const& triangle : triangles_) {
        for (size_t i = 0; i < 3; ++i) {
            auto st_index = gsl_lite::at(triangle.st, i);
            assert(st_index < texcoords_.size());
            auto const& st = texcoords_[st_index];
            auto const s =
                static_cast<float>(st.s) / static_cast<float>(hdr_.skinwidth);
            auto const t =
                static_cast<float>(st.t) / static_cast<float>(hdr_.skinheight);
            scaled_texcoords_.emplace_back(s, t);
        }
    }

    return infile.good();
}

bool MD2::load_frames(std::ifstream& infile, size_t offset) {
    gsl_Expects(infile);

    frames_.resize(hdr_.num_frames);
    key_frames_.resize(hdr_.num_frames);
    infile.seekg(gsl_lite::narrow<std::streamoff>(offset + hdr_.offset_frames));

    Animation current_anim;
    current_anim.start_frame = -1;

    for (auto i = 0; i < hdr_.num_frames; ++i) {
        auto& frame = frames_[i];

        frame.vertices.resize(
            hdr_.num_xyz); // same # of vertices for each keyframe

        infile.read(reinterpret_cast<char*>(frame.scale.data()),
                    sizeof(frame.scale));
        infile.read(reinterpret_cast<char*>(frame.translate.data()),
                    sizeof(frame.translate));
        infile.read(reinterpret_cast<char*>(frame.name.data()),
                    sizeof(frame.name));
        infile.read(
            reinterpret_cast<char*>(frame.vertices.data()),
            gsl_lite::narrow<std::streamsize>(sizeof(Vertex) * hdr_.num_xyz));

        std::string anim_id =
            animation_id_from_frame_name(std::string(frame.name.data()));

        if (current_anim.name == anim_id) {
            current_anim.end_frame = i;
        } else {
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
        auto& key_frame = key_frames_.at(i);
        key_frame.vertices.reserve(hdr_.num_tris * 3);

        for (auto const& triangle : triangles_) {
            for (size_t i = 0; i < 3; ++i) {
                auto vertex_index = gsl_lite::at(triangle.vertex, i);
                assert(vertex_index < frame.vertices.size());
                auto const& vertex = frame.vertices[vertex_index];
                auto const x = (frame.scale[0] *
                                gsl_lite::narrow_cast<float>(vertex.v[0])) +
                               frame.translate[0];
                auto const z = (frame.scale[1] *
                                gsl_lite::narrow_cast<float>(vertex.v[1])) +
                               frame.translate[1];
                auto const y = (frame.scale[2] *
                                gsl_lite::narrow_cast<float>(vertex.v[2])) +
                               frame.translate[2];
                key_frame.vertices.emplace_back(x, y, z);
            }
        }
        assert(key_frame.vertices.size() ==
               static_cast<size_t>(hdr_.num_tris * 3));
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

    interpolated_vertices_ = key_frames_.at(0).vertices;

    return infile.good();
}

void MD2::setup_buffers() {
    glGenVertexArrays(1, &vao_);
    glGenBuffers(2, vbo_.data());

    glBindVertexArray(vao_);
    spdlog::debug("vertex buffers: {} {}", vbo_[0], vbo_[1]);

    auto const& vertices = interpolated_vertices_;

    spdlog::info("num xyz: {}", vertices.size());
    static_assert(sizeof(glm::vec3) == 12, "bad vec3 size");

    glBindBuffer(GL_ARRAY_BUFFER, vbo_[0]);
    glBufferData(
        GL_ARRAY_BUFFER,
        gsl_lite::narrow_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3)),
        vertices.data(), GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    static_assert(sizeof(glm::vec2) == 8, "bad vec2 size");

    glBindBuffer(GL_ARRAY_BUFFER, vbo_[1]);
    glBufferData(GL_ARRAY_BUFFER,
                 gsl_lite::narrow_cast<GLsizeiptr>(scaled_texcoords_.size() *
                                                   sizeof(glm::vec2)),
                 scaled_texcoords_.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindVertexArray(0);

    glCheckError();
}

void MD2::set_animation(std::string const& id) {
    auto iter = animation_index_map_.find(id);

    if (iter != animation_index_map_.end()) {
        auto index = iter->second;
        set_animation(index);
    }
}

void MD2::set_animation(size_t index) {
    gsl_Expects(index < animations_.size());

    if (std::cmp_not_equal(current_animation_index_, index)) {
        auto const& anim = animations_[index];
        next_frame_ = anim.start_frame;
        current_animation_index_ = index;
        interpolation_ = 0.0;
    }
}

void MD2::set_skin_index(size_t index) {
    gsl_Expects(index < skins_.size());
    current_skin_index_ = index;
}

void MD2::update(float dt) {
    auto const& anim = gsl_lite::at(animations_, current_animation_index_);
    auto const paused = frames_per_second_ == 0.0f;
    auto const single_frame = anim.start_frame == anim.end_frame;
    auto const done = !anim.loop && current_frame_ == anim.end_frame;

    if (paused || single_frame || done) {
        return;
    }

    interpolation_ += dt * frames_per_second_;

    if (interpolation_ >= 1.0f) {
        current_frame_ = next_frame_;
        ++next_frame_;
        interpolation_ = 0.0f;

        if (next_frame_ > anim.end_frame) {
            next_frame_ = anim.start_frame;
        }
    }

    float t = interpolation_;
    int i = 0;

    for (auto& vertex : interpolated_vertices_) {
        auto const& v1 = key_frames_.at(current_frame_).vertices[i];
        auto const& v2 = key_frames_.at(next_frame_).vertices[i];
        vertex = lerp(v1, v2, glm::vec3(t, t, t));
        ++i;
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo_[0]);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    gsl_lite::narrow_cast<GLsizeiptr>(
                        sizeof(glm::vec3) * interpolated_vertices_.size()),
                    interpolated_vertices_.data());
}

void MD2::draw(Shader& /* shader */) {
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0,
                 gsl_lite::narrow_cast<GLsizei>(interpolated_vertices_.size()));
    glCheckError();
}

bool MD2::validate_header(Header const& hdr) {
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

bool MD2::draw_ui() {
    int index = gsl_lite::narrow_cast<int>(animation_index());
    constexpr int anim_max_height_in_items = 15;

    ImGui::Combo(
        "Animation", &index,
        [](void* data, int idx) -> char const* {
            auto const* md2 = static_cast<MD2 const*>(data);
            gsl_Assert(static_cast<size_t>(idx) < md2->animations().size());
            return md2->animations()[idx].name.c_str();
        },
        this, gsl_lite::narrow_cast<int>(animations().size()),
        anim_max_height_in_items);

    set_animation(static_cast<size_t>(index));

    int sindex = gsl_lite::narrow_cast<int>(skin_index());

    ImGui::Combo(
        "Skin", &sindex,
        [](void* data, int idx) -> char const* {
            auto const* md2 = static_cast<MD2 const*>(data);
            gsl_Assert(static_cast<size_t>(idx) < md2->skins().size());
            return md2->skins()[idx].name.c_str();
        },
        this, gsl_lite::narrow_cast<int>(skins().size()));

    float fps = frames_per_second();
    ImGui::InputFloat("Animation FPS", &fps, 1.0f, 5.0f, "%.3f");
    set_frames_per_second(fps);

    if (static_cast<size_t>(sindex) == skin_index()) {
        return false;
    }

    set_skin_index(static_cast<size_t>(sindex));
    return true;
}

std::ostream& operator<<(std::ostream& os, MD2::Animation const& anim) {
    os << '\n'
       << "id:    " << anim.name << '\n'
       << "start: " << anim.start_frame << '\n'
       << "end:   " << anim.end_frame << '\n'
       << "loop:  " << std::boolalpha << anim.loop << '\n';

    return os;
}

std::ostream& operator<<(std::ostream& os, MD2::Header const& hdr) {
    os << '\n'
       << "ident:         " << hdr.ident << '\n'
       << "version:       " << hdr.version << '\n'
       << "skinwidth:     " << hdr.skinwidth << '\n'
       << "skinheight:    " << hdr.skinheight << '\n'
       << "framesize:     " << hdr.framesize << '\n'
       << "num_skins:     " << hdr.num_skins << '\n'
       << "num_xyz:       " << hdr.num_xyz << '\n'
       << "num_st:        " << hdr.num_st << '\n'
       << "num_tris:      " << hdr.num_tris << '\n'
       << "num_glcmds:    " << hdr.num_glcmds << '\n'
       << "num_frames:    " << hdr.num_frames << '\n'
       << "offset_skins:  " << hdr.offset_skins << '\n'
       << "offset_st:     " << hdr.offset_st << '\n'
       << "offset_tris:   " << hdr.offset_tris << '\n'
       << "offset_frames: " << hdr.offset_frames << '\n'
       << "offset_glcmds: " << hdr.offset_glcmds << '\n'
       << "offset_end:    " << hdr.offset_end;

    return os;
}
