#pragma once

#include "md2view/gl/gl.hpp"

#include <optional>
#include <vector>

class FrameBuffer {
public:
    FrameBuffer() = default;
    FrameBuffer(GLuint width,
                GLuint height,
                size_t num_color_buffers = 1,
                bool enable_render_buffer = false);
    ~FrameBuffer();

    FrameBuffer(FrameBuffer const&) = delete;
    FrameBuffer& operator=(FrameBuffer const&) = delete;

    FrameBuffer(FrameBuffer&&) = delete;
    FrameBuffer& operator=(FrameBuffer&&) = delete;

    void bind() const { glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_); }

    static void bind_default() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

    [[nodiscard]] GLuint color_buffer(size_t n) const {
        return color_buffers_.at(n);
    }
    void use_color_buffer(size_t n) const {
        glBindTexture(GL_TEXTURE_2D, color_buffers_.at(n));
    }

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
