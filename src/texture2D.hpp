#pragma once

#include "gl.hpp"

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

    GLuint width() const { return width_; }
    GLuint height() const { return height_; }

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
