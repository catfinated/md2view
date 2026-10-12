#include "md2view/md2.hpp"
#include "md2view/pak.hpp"

#include <fmt/base.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtx/compatibility.hpp>
#include <gsl-lite/gsl-lite.hpp>
#include <spdlog/spdlog.h>

#include <array>
#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

template <> struct fmt::formatter<MD2::Header> : ostream_formatter {};
template <> struct fmt::formatter<MD2::Animation> : ostream_formatter {};

std::string animationIdFromFrameName(std::string const& name) {
    std::string id;

    for (auto const& ch : name) {
        if (isdigit(ch) != 0) {
            break;
        }

        id += ch;
    }

    return id;
}

MD2::MD2(std::string_view filename, PAK const& pak) {
    if (!load(pak, filename)) {
        throw std::runtime_error(
            fmt::format("failed to load MD2 model {}", filename));
    }
}

bool MD2::load(PAK const& pf, std::string_view filename) {
    spdlog::info("loading model {} from pak {}", filename, pf.fpath());
    gsl_Expects(!filename.empty());
    auto ispak = !pf.isDirectory();

    {
        auto inf = pf.openIfstream(filename);
        gsl_Expects(inf);
        if (!load(inf)) {
            return false;
        }
    }

    if (!ispak) {
        auto p = pf.fpath() / filename;
        loadSkinsFromDirectory(p.parent_path(), pf.fpath());
    }
    return true;
}

void MD2::loadSkinsFromDirectory(std::filesystem::path const& dpath,
                                 std::filesystem::path const& root) {
    static constexpr std::array extensions = {".pcx", ".png", ".jpg"};

    spdlog::info("load skins from {}", dpath);
    std::vector<SkinData> foundSkins;
    for (auto const& skin : skins_) {
        auto path = dpath / skin.name;
        for (auto const& ext : extensions) {
            path = path.replace_extension(ext);
            if (std::filesystem::exists(path)) {
                foundSkins.emplace_back(path.lexically_relative(root).string(),
                                        path.stem().string());
                break;
            }
        }
    }

    if (foundSkins.empty()) { // drfreak model has no skins specified
        for (auto const& dirEntry :
             std::filesystem::directory_iterator{dpath}) {
            if (std::filesystem::is_regular_file(dirEntry.path())) {
                auto const extension = dirEntry.path().extension();
                if (extension == ".png") {
                    auto relpath = dirEntry.path().lexically_relative(root);
                    foundSkins.emplace_back(relpath.string(),
                                            relpath.stem().string());
                }
            }
        }
    }
    skins_ = std::move(foundSkins);
}

bool MD2::load(std::ifstream& infile) {
    nextFrame_ = 1;
    currentFrame_ = 0;
    interpolation_ = 0.0f;

    static_assert(sizeof(hdr_) == (17 * sizeof(int32_t)),
                  "md2 header has padding");

    size_t offset = infile.tellg(); // header offset
    infile.read(reinterpret_cast<char*>(&hdr_), sizeof(hdr_));
    spdlog::debug("md2 header: {}", hdr_);

    gsl_Assert(loadSkins(infile, offset));
    gsl_Assert(loadTriangles(infile, offset));
    gsl_Assert(loadTexcoords(infile, offset));
    gsl_Assert(loadFrames(infile, offset));

    return true;
}

bool MD2::loadSkins(std::ifstream& infile, size_t offset) {
    gsl_Expects(infile);

    // read skins
    static_assert(sizeof(Skin) == 64, "md2 skin has padding");
    std::vector<Skin> skins;
    skins.resize(hdr_.numSkins); // need resize so we actually push elements
    infile.seekg(gsl_lite::narrow<std::streamoff>(offset + hdr_.offsetSkins));
    infile.read(reinterpret_cast<char*>(skins.data()),
                gsl_lite::narrow<std::streamsize>(sizeof(Skin) * skins.size()));
    assert(static_cast<size_t>(infile.gcount()) == sizeof(Skin) * skins.size());
    assert(skins.size() == static_cast<size_t>(hdr_.numSkins));
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

bool MD2::loadTriangles(std::ifstream& infile, size_t offset) {
    gsl_Expects(infile);
    static_assert(sizeof(Triangle) == 6 * sizeof(uint16_t),
                  "md2 triangle has padding");
    triangles_.resize(hdr_.numTris);
    infile.seekg(gsl_lite::narrow<std::streamoff>(offset + hdr_.offsetTris));
    infile.read(reinterpret_cast<char*>(triangles_.data()),
                gsl_lite::narrow<std::streamsize>(sizeof(Triangle) *
                                                  triangles_.size()));

    return infile.good();
}

bool MD2::loadTexcoords(std::ifstream& infile, size_t offset) {
    gsl_Expects(infile);
    gsl_Expects(!triangles_.empty());

    // read texcoords
    static_assert(sizeof(TexCoord) == 2 * sizeof(int16_t),
                  "md2 texcoord has padding");
    texcoords_.resize(hdr_.numSt);
    infile.seekg(gsl_lite::narrow<std::streamoff>((offset + hdr_.offsetSt)));
    infile.read(reinterpret_cast<char*>(texcoords_.data()),
                gsl_lite::narrow<std::streamsize>(sizeof(TexCoord) *
                                                  texcoords_.size()));

    // we must scale the texcoords and unpack the triangles
    // into a flat vector to better work gith glDrawArrays
    scaledTexcoords_.reserve(hdr_.numTris * 3);

    for (auto const& triangle : triangles_) {
        for (size_t i = 0; i < 3; ++i) {
            auto stIndex = gsl_lite::at(triangle.st, i);
            if (stIndex >= texcoords_.size()) {
                throw std::runtime_error(fmt::format(
                    "malformed MD2: texture coordinate index {} out of range "
                    "({} texture coordinates)",
                    stIndex, texcoords_.size()));
            }
            auto const& st = texcoords_[stIndex];
            auto const s =
                static_cast<float>(st.s) / static_cast<float>(hdr_.skinwidth);
            auto const t =
                static_cast<float>(st.t) / static_cast<float>(hdr_.skinheight);
            scaledTexcoords_.emplace_back(s, t);
        }
    }

    return infile.good();
}

// key frame contains the scaled vertices unpacked from the triangles
// into a flat vector since same vertex can have a different tex coord
// in two different triangles. therefore, KeyFrame has num_tris * 3 entries
// and tex coord buffer ends up with num_tris * 3 entries too but
// is shared by all frames.
MD2::KeyFrame MD2::unpackFrame(Frame const& frame) const {
    KeyFrame keyFrame;
    keyFrame.vertices.reserve(triangles_.size() * 3);

    for (auto const& triangle : triangles_) {
        for (auto const vertexIndex : triangle.vertex) {
            if (vertexIndex >= frame.vertices.size()) {
                throw std::runtime_error(fmt::format(
                    "malformed MD2: vertex index {} out of range ({} "
                    "vertices)",
                    vertexIndex, frame.vertices.size()));
            }
            auto const& vertex = frame.vertices[vertexIndex];
            auto const x =
                (frame.scale[0] * gsl_lite::narrow_cast<float>(vertex.v[0])) +
                frame.translate[0];
            auto const z =
                (frame.scale[1] * gsl_lite::narrow_cast<float>(vertex.v[1])) +
                frame.translate[1];
            auto const y =
                (frame.scale[2] * gsl_lite::narrow_cast<float>(vertex.v[2])) +
                frame.translate[2];
            keyFrame.vertices.emplace_back(x, y, z);
        }
    }
    assert(keyFrame.vertices.size() == static_cast<size_t>(hdr_.numTris * 3));
    return keyFrame;
}

bool MD2::loadFrames(std::ifstream& infile, size_t offset) {
    gsl_Expects(infile);

    frames_.resize(hdr_.numFrames);
    keyFrames_.resize(hdr_.numFrames);
    infile.seekg(gsl_lite::narrow<std::streamoff>(offset + hdr_.offsetFrames));

    Animation currentAnim;
    currentAnim.startFrame = -1;

    for (auto i = 0; i < hdr_.numFrames; ++i) {
        auto& frame = frames_[i];

        frame.vertices.resize(
            hdr_.numXyz); // same # of vertices for each keyframe

        infile.read(reinterpret_cast<char*>(frame.scale.data()),
                    sizeof(frame.scale));
        infile.read(reinterpret_cast<char*>(frame.translate.data()),
                    sizeof(frame.translate));
        infile.read(reinterpret_cast<char*>(frame.name.data()),
                    sizeof(frame.name));
        infile.read(
            reinterpret_cast<char*>(frame.vertices.data()),
            gsl_lite::narrow<std::streamsize>(sizeof(Vertex) * hdr_.numXyz));

        std::string animId =
            animationIdFromFrameName(std::string(frame.name.data()));

        if (currentAnim.name == animId) {
            currentAnim.endFrame = i;
        } else {
            if (currentAnim.startFrame != -1) {
                animationIndexMap_[currentAnim.name] = animations_.size();
                animations_.push_back(currentAnim);
            }

            currentAnim.startFrame = i;
            currentAnim.endFrame = i;
            currentAnim.name = animId;
        }

        keyFrames_.at(i) = unpackFrame(frame);
    }

    assert(keyFrames_.size() == frames_.size());
    assert(frames_.size() == static_cast<size_t>(hdr_.numFrames));

    if (currentAnim.startFrame != -1) {
        animationIndexMap_[currentAnim.name] = animations_.size();
        animations_.push_back(currentAnim);
    }

    for (auto const& anim : animations_) {
        spdlog::debug("animation: {}", anim);
    }

    interpolatedVertices_ = keyFrames_.at(0).vertices;

    return infile.good();
}

void MD2::setAnimation(std::string const& id) {
    auto iter = animationIndexMap_.find(id);

    if (iter != animationIndexMap_.end()) {
        auto index = iter->second;
        setAnimation(index);
    }
}

void MD2::setAnimation(size_t index) {
    gsl_Expects(index < animations_.size());

    if (std::cmp_not_equal(currentAnimationIndex_, index)) {
        auto const& anim = animations_[index];
        nextFrame_ = anim.startFrame;
        currentAnimationIndex_ = index;
        interpolation_ = 0.0;
    }
}

void MD2::setSkinIndex(size_t index) {
    gsl_Expects(index < skins_.size());
    currentSkinIndex_ = index;
}

void MD2::update(float dt) {
    auto const& anim = gsl_lite::at(animations_, currentAnimationIndex_);
    auto const paused = framesPerSecond_ == 0.0f;
    auto const singleFrame = anim.startFrame == anim.endFrame;
    auto const done = !anim.loop && currentFrame_ == anim.endFrame;

    if (paused || singleFrame || done) {
        return;
    }

    interpolation_ += dt * framesPerSecond_;

    if (interpolation_ >= 1.0f) {
        currentFrame_ = nextFrame_;
        ++nextFrame_;
        interpolation_ = 0.0f;

        if (nextFrame_ > anim.endFrame) {
            nextFrame_ = anim.startFrame;
        }
    }

    float t = interpolation_;
    int i = 0;

    for (auto& vertex : interpolatedVertices_) {
        auto const& v1 = keyFrames_.at(currentFrame_).vertices[i];
        auto const& v2 = keyFrames_.at(nextFrame_).vertices[i];
        vertex = lerp(v1, v2, glm::vec3(t, t, t));
        ++i;
    }
}

std::ostream& operator<<(std::ostream& os, MD2::Animation const& anim) {
    os << '\n'
       << "id:    " << anim.name << '\n'
       << "start: " << anim.startFrame << '\n'
       << "end:   " << anim.endFrame << '\n'
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
       << "num_skins:     " << hdr.numSkins << '\n'
       << "num_xyz:       " << hdr.numXyz << '\n'
       << "num_st:        " << hdr.numSt << '\n'
       << "num_tris:      " << hdr.numTris << '\n'
       << "num_glcmds:    " << hdr.numGlcmds << '\n'
       << "num_frames:    " << hdr.numFrames << '\n'
       << "offset_skins:  " << hdr.offsetSkins << '\n'
       << "offset_st:     " << hdr.offsetSt << '\n'
       << "offset_tris:   " << hdr.offsetTris << '\n'
       << "offset_frames: " << hdr.offsetFrames << '\n'
       << "offset_glcmds: " << hdr.offsetGlcmds << '\n'
       << "offset_end:    " << hdr.offsetEnd;

    return os;
}
