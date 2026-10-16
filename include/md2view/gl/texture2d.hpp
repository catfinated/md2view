/// @file
/// OpenGL 2D texture
#pragma once

#include "md2view/gl/gl.hpp"

#include <span>

class PAK;
class Image;

namespace GL {

/// An OpenGL 2D texture wrapping a single `GL_TEXTURE_2D` object.
class Texture2D {
public:
    /// Sampling and wrap parameters applied at upload time.
    struct Attributes {
        GLuint imageFormat = GL_RGBA;    ///< Source pixel format.
        GLint internalFormat = GL_RGBA8; ///< GPU internal storage format.
        GLuint wrapS = GL_REPEAT;        ///< Horizontal wrap mode.
        GLuint wrapT = GL_REPEAT;        ///< Vertical wrap mode.
        GLint filterMin = GL_LINEAR;     ///< Minification filter.
        GLint filterMax = GL_LINEAR;     ///< Magnification filter.
    };

    /// Upload pixel data to the GPU.
    ///
    /// @param image The image to upload
    explicit Texture2D(Image const& image);
    ~Texture2D();

    Texture2D(Texture2D const&) = delete;
    Texture2D& operator=(Texture2D const&) = delete;

    Texture2D(Texture2D&& rhs) noexcept;
    Texture2D& operator=(Texture2D&& rhs) noexcept;

    /// Bind to the currently active texture unit.
    void bind() const;

    [[nodiscard]] Attributes const& attributes() const { return attr_; }
    [[nodiscard]] GLuint id() const { return id_; }
    [[nodiscard]] GLuint width() const { return width_; }
    [[nodiscard]] GLuint height() const { return height_; }

    /// Unbind any texture from `GL_TEXTURE_2D`.
    static void unbind() { glBindTexture(GL_TEXTURE_2D, 0); }

private:
    void cleanup();
    [[nodiscard]] bool
    init(GLuint width, GLuint height, std::span<unsigned char const> data);

    Attributes attr_;
    GLuint id_{};
    GLuint width_{};
    GLuint height_{};
};

} // namespace GL
