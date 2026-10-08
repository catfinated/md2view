#include "md2view/gl/texture2d.hpp"
#include "md2view/image.hpp"
#include "md2view/pak.hpp"

#include <gsl-lite/gsl-lite.hpp>
#include <spdlog/spdlog.h>

#include <stdexcept>
#include <utility>

namespace GL {

Texture2D::Texture2D(Image const& image) {
    if (!init(image.width(), image.height(), image.data())) {
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
                     std::span<unsigned char const> data) {
    glGenTextures(1, &id_);

    width_ = width;
    height_ = height;
    attr_.filter_min = GL_LINEAR_MIPMAP_LINEAR;

    bind();

    spdlog::debug("init 2D texture {}x{}", width_, height_);

    glTexImage2D(GL_TEXTURE_2D, 0, attr_.internal_format,
                 gsl_lite::narrow_cast<GLsizei>(width_),
                 gsl_lite::narrow_cast<GLsizei>(height_), 0, attr_.image_format,
                 GL_UNSIGNED_BYTE, data.data());

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

} // namespace GL
