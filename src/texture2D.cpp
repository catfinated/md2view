#include "md2view/texture2D.hpp"
#include "md2view/pak.hpp"
#include "md2view/pcx.hpp"

#include <gsl-lite/gsl-lite.hpp>
#include <spdlog/spdlog.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <stdexcept>
#include <utility>

Texture2D::Texture2D(GLuint width,
                     GLuint height,
                     std::span<unsigned char const> data,
                     bool alpha) {
    if (!init(width, height, data, alpha)) {
        throw std::runtime_error("failed to init Texture2D");
    }
}

Texture2D::~Texture2D() { cleanup(); }

Texture2D::Texture2D(Texture2D&& rhs) noexcept
    : attr_{rhs.attr_}
    , width_{rhs.width_}
    , height_{rhs.height_} {
    id_ = std::exchange(rhs.id_, 0U);
}

Texture2D& Texture2D::operator=(Texture2D&& rhs) noexcept {
    if (this != &rhs) {
        cleanup();
        attr_ = rhs.attr_;
        id_ = std::exchange(rhs.id_, 0U);
        width_ = rhs.width_;
        height_ = rhs.height_;
    }

    return *this;
}

void Texture2D::cleanup() {
    if (id_ != 0U) {
        glDeleteTextures(1, &id_);
        id_ = 0U;
    }
}

bool Texture2D::init(GLuint width,
                     GLuint height,
                     std::span<unsigned char const> data,
                     bool alpha) {
    glGenTextures(1, &id_);

    width_ = width;
    height_ = height;

    attr_.internal_format = alpha ? GL_RGBA : GL_RGB8;
    attr_.image_format = alpha ? GL_RGBA : GL_RGB;
    attr_.filter_min = GL_LINEAR_MIPMAP_LINEAR;
    attr_.filter_max = GL_LINEAR;

    bind();

    spdlog::debug("init 2D texture {}x{}", width_, height_);

    glTexImage2D(GL_TEXTURE_2D, 0, attr_.internal_format, width_, height_, 0,
                 attr_.image_format, GL_UNSIGNED_BYTE, data.data());

    glGenerateMipmap(GL_TEXTURE_2D);

    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, attr_.wrap_s);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, attr_.wrap_t);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, attr_.filter_min);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, attr_.filter_max);

    spdlog::info("initialized 2D texture");

    unbind();

    glCheckError();

    return true;
}

void Texture2D::bind() const { glBindTexture(GL_TEXTURE_2D, id_); }

std::shared_ptr<Texture2D> Texture2D::load(PAK const& pak,
                                           std::string const& path) {
    spdlog::info("load texture {} from {}", path, pak.fpath().string());
    auto const is_pcx = std::filesystem::path(path).extension() == ".pcx";

    if (is_pcx) {
        auto inf = pak.open_ifstream(path);
        gsl_Assert(inf.is_open());
        PCX pcx(inf);
        return std::make_shared<Texture2D>(pcx.width(), pcx.height(),
                                           std::span{pcx.image()});
    }

    auto const abspath = (pak.fpath() / path).make_preferred();
    gsl_Assert(std::filesystem::exists(abspath));
    int width{};
    int height{};
    int n{};
    unsigned char* image =
        stbi_load(abspath.string().c_str(), &width, &height, &n, 3);
    gsl_Assert(image);
    auto texture = std::make_shared<Texture2D>(
        width, height, std::span{image, static_cast<size_t>(width * height)});
    spdlog::info("loaded 2D texture {} width: {} height: {}", path, width,
                 height);
    stbi_image_free(image);
    return texture;
}
