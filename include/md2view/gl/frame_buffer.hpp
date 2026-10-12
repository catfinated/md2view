#pragma once

#include "md2view/gl/gl.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace GL {

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
                size_t numColorBuffers = 1,
                bool enableRenderBuffer = false);

    ~FrameBuffer();

    FrameBuffer(FrameBuffer const&) = delete;
    FrameBuffer& operator=(FrameBuffer const&) = delete;
    FrameBuffer(FrameBuffer&&) = delete;
    FrameBuffer& operator=(FrameBuffer&&) = delete;

    /// Bind this FBO as the current draw target.
    void bind() const { glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer_); }

    /// Restore the default framebuffer (the window surface).
    static void bindDefault() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

    /// Return the OpenGL texture handle for colour attachment @p n.
    [[nodiscard]] GLuint colorBuffer(size_t n) const {
        return colorBuffers_.at(n);
    }

    /// Bind colour attachment @p n to the current texture unit.
    void useColorBuffer(size_t n) const {
        glBindTexture(GL_TEXTURE_2D, colorBuffers_.at(n));
    }

    /// The underlying FBO handle.
    [[nodiscard]] GLuint handle() const { return frameBuffer_; }

private:
    void cleanup();
    void createTextureAttachment(GLuint width, GLuint height);
    void createRenderBufferAttachement(GLuint width, GLuint height);
    [[nodiscard]] bool
    init(GLuint width, GLuint height, bool enableRenderBuffer);

    GLuint frameBuffer_{};
    std::vector<GLuint> colorBuffers_;
    std::optional<GLuint> renderBuffer_;
};

} // namespace GL
