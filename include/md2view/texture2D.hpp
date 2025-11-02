#pragma once

#include "gl.hpp"

#include <memory>
#include <span>
#include <string>

class PAK;

class Texture2D {
public:
    struct Attributes {
        GLuint image_format = GL_RGB;
        GLint internal_format = GL_RGB;
        GLuint wrap_s = GL_REPEAT;
        GLuint wrap_t = GL_REPEAT;
        GLint filter_min = GL_LINEAR;
        GLint filter_max = GL_LINEAR;
    };

    ~Texture2D();
    Texture2D(GLuint width,
              GLuint height,
              std::span<unsigned char const> data,
              bool alpha = false);

    // non-copyable
    Texture2D(Texture2D const&) = delete;
    Texture2D& operator=(Texture2D const&) = delete;

    // movable
    Texture2D(Texture2D&& rhs) noexcept;
    Texture2D& operator=(Texture2D&& rhs) noexcept;

    void bind() const;

    [[nodiscard]] Attributes const& attributes() const { return attr_; }
    [[nodiscard]] GLuint id() const { return id_; }

    [[nodiscard]] GLuint width() const { return width_; }
    [[nodiscard]] GLuint height() const { return height_; }

    static void unbind() { glBindTexture(GL_TEXTURE_2D, 0); }

    static std::shared_ptr<Texture2D> load(PAK const& pak,
                                           std::string const& path);

private:
    void cleanup();
    [[nodiscard]] bool init(GLuint width,
                            GLuint height,
                            std::span<unsigned char const> data,
                            bool alpha);

    Attributes attr_;
    GLuint id_{};
    GLuint width_{};
    GLuint height_{};
}; // class Texture2D
