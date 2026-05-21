#pragma once

#include "md2view/gl/gl.hpp"

#include <optional>
#include <vector>

/// OpenGL framebuffer object (FBO) with one or more colour attachments.
///
/// Supports multi-target rendering (multiple colour texture attachments) and an
/// optional renderbuffer depth attachment. Used for the main scene pass and the
/// blur pass in the glow post-processing pipeline.
class FrameBuffer {
public:
    FrameBuffer() = default;

    /// Create an FBO with @p num_color_buffers texture colour attachments.
    ///
    /// @param width                Framebuffer width in pixels.
    /// @param height               Framebuffer height in pixels.
    /// @param num_color_buffers    Number of colour texture attachments.
    /// @param enable_render_buffer If true, attach a depth renderbuffer.
    FrameBuffer(GLuint width,
                GLuint height,
                size_t num_color_buffers = 1,
                bool enable_render_buffer = false);

    ~FrameBuffer();

    FrameBuffer(FrameBuffer const&) = delete;
    FrameBuffer& operator=(FrameBuffer const&) = delete;
    FrameBuffer(FrameBuffer&&) = delete;
    FrameBuffer& operator=(FrameBuffer&&) = delete;

    /// Bind this FBO as the current draw target.
    void bind() const { glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_); }

    /// Restore the default framebuffer (the window surface).
    static void bind_default() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

    /// Return the OpenGL texture handle for colour attachment @p n.
    [[nodiscard]] GLuint color_buffer(size_t n) const {
        return color_buffers_.at(n);
    }

    /// Bind colour attachment @p n to the current texture unit.
    void use_color_buffer(size_t n) const {
        glBindTexture(GL_TEXTURE_2D, color_buffers_.at(n));
    }

    /// The underlying FBO handle.
    [[nodiscard]] GLuint handle() const { return frame_buffer_; }

private:
    void cleanup();
    void create_texture_attachment(GLuint width, GLuint height);
    void create_render_buffer_attachement(GLuint width, GLuint height);
    [[nodiscard]] bool
    init(GLuint width, GLuint height, bool enable_render_buffer);

    GLuint frame_buffer_{};
    std::vector<GLuint> color_buffers_;
    std::optional<GLuint> render_buffer_;
};
