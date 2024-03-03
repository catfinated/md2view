#include "texture2D.hpp"
#include "common.hpp"

#include <spdlog/spdlog.h>

#include <stdexcept>

 Texture2D::Texture2D(GLuint width, GLuint height, unsigned char * data)
{
    if (!init(width, height, data)) {
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
    id_ = rhs.id_;
    width_ = rhs.width_;
    height_ = rhs.height_;
    initialized_ = rhs.initialized_;

    rhs.id_ = 0;
    rhs.initialized_ = false;
}

 Texture2D& Texture2D::operator=(Texture2D&& rhs)
{
    if (this != &rhs) {
        cleanup();
        attr_ = rhs.attr_;
        id_ = rhs.id_;
        width_ = rhs.width_;
        height_ = rhs.height_;
        initialized_ = rhs.initialized_;

        rhs.id_ = 0;
        rhs.initialized_ = false;
    }

    return *this;
}

 void Texture2D::cleanup()
{
    if (initialized_) {
        glDeleteTextures(1, &id_);
        initialized_ = false;
    }
}

 bool Texture2D::init(GLuint width, GLuint height, unsigned char const * data)
{
    if (initialized_) {
        return false;
    }

    glGenTextures(1, &id_);

    width_ = width;
    height_ = height;
    initialized_ = true;

    attr_.internal_format = GL_RGB8;
    attr_.image_format = GL_RGB;
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
    MD2V_EXPECT(initialized_);
    glBindTexture(GL_TEXTURE_2D, id_);
}

 void Texture2D::set_alpha(bool alpha)
{
    if (alpha) {
        attr_.internal_format = GL_RGBA;
        attr_.image_format = GL_RGBA;
    }
    else {
        attr_.internal_format = GL_RGB;
        attr_.image_format = GL_RGB;
    }
}
