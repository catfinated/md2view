#pragma once

#include "common.hpp"

class Texture2D
{
public:
    struct Attributes
    {
        GLuint image_format = GL_RGB;
        GLuint internal_format = GL_RGB;
        GLuint wrap_s = GL_REPEAT;
        GLuint wrap_t = GL_REPEAT;
        GLuint filter_min = GL_LINEAR;
        GLuint filter_max = GL_LINEAR;
    };

    Texture2D() = default;
    ~Texture2D();
    Texture2D(GLuint width, GLuint height, unsigned char * data);

    // non-copyable
    Texture2D(Texture2D const&) = delete;
    Texture2D& operator=(Texture2D const&) = delete;

    // movable
    Texture2D(Texture2D&& rhs);
    Texture2D& operator=(Texture2D&&);

    bool init(GLuint width, GLuint height, unsigned char const * data);
    void bind() const;

    Attributes const& attributes() const { return attr_; }
    void set_alpha(bool alpha);
    bool initialized() const { return initialized_; }
    GLuint id() const { return id_; }

    static void unbind() { glBindTexture(GL_TEXTURE_2D, 0); }

private:
    void cleanup();

private:
    Attributes attr_;
    GLuint id_;
    GLuint width_ = 0;
    GLuint height_ = 0;
    bool initialized_ = false;

}; // class Texture2D

inline Texture2D::Texture2D(GLuint width, GLuint height, unsigned char * data)
{
    if (!init(width, height, data)) {
        throw std::runtime_error("failed to init Texture2D");
    }
}

inline Texture2D::~Texture2D()
{
    cleanup();
}

inline Texture2D::Texture2D(Texture2D&& rhs)
{
    attr_ = rhs.attr_;
    id_ = rhs.id_;
    width_ = rhs.width_;
    height_ = rhs.height_;
    initialized_ = rhs.initialized_;

    rhs.id_ = 0;
    rhs.initialized_ = false;
}

inline Texture2D& Texture2D::operator=(Texture2D&& rhs)
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

inline void Texture2D::cleanup()
{
    if (initialized_) {
        glDeleteTextures(1, &id_);
        initialized_ = false;
    }
}

inline bool Texture2D::init(GLuint width, GLuint height, unsigned char const * data)
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

    std::cout << width_ <<  " x " << height_ << '\n';

    glTexImage2D(GL_TEXTURE_2D, 0, attr_.internal_format,
                 width_, height_, 0, attr_.image_format,
                 GL_UNSIGNED_BYTE, data);

    glGenerateMipmap(GL_TEXTURE_2D);

    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, attr_.wrap_s);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, attr_.wrap_t);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, attr_.filter_min);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, attr_.filter_max);

    std::cout << "initialized 2D texture\n";

    unbind();

    glCheckError();

    return true;
}

inline void Texture2D::bind() const
{
    BLUE_EXPECT(initialized_);
    glBindTexture(GL_TEXTURE_2D, id_);
}

inline void Texture2D::set_alpha(bool alpha)
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
