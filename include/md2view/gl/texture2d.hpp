#pragma once

#include "md2view/gl/gl.hpp"

#include <memory>
#include <span>
#include <string>

class PAK;

namespace GL {

/// An OpenGL 2D texture wrapping a single `GL_TEXTURE_2D` object.
class Texture2D {
public:
    /// Sampling and wrap parameters applied at upload time.
    struct Attributes {
        GLuint image_format = GL_RGB;   ///< Source pixel format.
        GLint internal_format = GL_RGB; ///< GPU internal storage format.
        GLuint wrap_s = GL_REPEAT;      ///< Horizontal wrap mode.
        GLuint wrap_t = GL_REPEAT;      ///< Vertical wrap mode.
        GLint filter_min = GL_LINEAR;   ///< Minification filter.
        GLint filter_max = GL_LINEAR;   ///< Magnification filter.
    };

    ~Texture2D();

    /// Upload pixel data to the GPU.
    ///
    /// @param width  Image width in pixels.
    /// @param height Image height in pixels.
    /// @param data   Raw pixel bytes (RGB or RGBA depending on @p alpha).
    /// @param alpha  If true, treats @p data as RGBA and enables alpha
    /// blending.
    Texture2D(GLuint width,
              GLuint height,
              std::span<unsigned char const> data,
              bool alpha = false);

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

    /// Load a texture from a PAK entry, decoding PCX or common image formats.
    ///
    /// @param pak  The archive to load from.
    /// @param path Archive-relative path to the image file.
    /// @return A heap-allocated Texture2D ready for use.
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
};

} // namespace GL
