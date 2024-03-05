#include "texture2D.hpp"
#include "common.hpp"

#include <spdlog/spdlog.h>

#include <stdexcept>
#include <utility>

 Texture2D::Texture2D(GLuint width, GLuint height, unsigned char const * data, bool alpha)
{
    if (!init(width, height, data, alpha)) {
        throw std::runtime_error("failed to init Texture2D");
    }
}

 Texture2D::~Texture2D()
{
    cleanup();
}

 Texture2D::Texture2D(Texture2D&& rhs)
{
    attr_ = rhs.attr_;
    id_ = std::exchange(rhs.id_, 0U);
    width_ = rhs.width_;
    height_ = rhs.height_;
}

 Texture2D& Texture2D::operator=(Texture2D&& rhs)
{
    if (this != &rhs) {
        cleanup();
        attr_ = rhs.attr_;
        id_ = std::exchange(rhs.id_, 0U);
        width_ = rhs.width_;
        height_ = rhs.height_;
    }

    return *this;
}

 void Texture2D::cleanup()
{
    if (id_ != 0U) {
        glDeleteTextures(1, &id_);
        id_ = 0U;
    }
}

 bool Texture2D::init(GLuint width, GLuint height, unsigned char const * data, bool alpha)
{
    glGenTextures(1, &id_);

    width_ = width;
    height_ = height;

    attr_.internal_format = alpha ? GL_RGBA : GL_RGB8;
    attr_.image_format = alpha ? GL_RGBA : GL_RGB;
    attr_.filter_min = GL_LINEAR_MIPMAP_LINEAR;
    attr_.filter_max = GL_LINEAR;

    bind();

    spdlog::debug("init 2D texture {}x{}", width_, height_);

    glTexImage2D(GL_TEXTURE_2D, 0, attr_.internal_format,
                 width_, height_, 0, attr_.image_format,
                 GL_UNSIGNED_BYTE, data);

    glGenerateMipmap(GL_TEXTURE_2D);

    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, attr_.wrap_s);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, attr_.wrap_t);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, attr_.filter_min);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, attr_.filter_max);

    spdlog::info("initialized 2D texture");

    unbind();

    glCheckError();

    return true;
}

 void Texture2D::bind() const
{
    glBindTexture(GL_TEXTURE_2D, id_);
}

 