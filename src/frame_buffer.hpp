#pragma once

#include "gl.hpp"

#include <boost/optional.hpp>

#include <stdexcept>

template <size_t NumColorBuf = 1, bool RenderBuf = false>
class FrameBuffer
{
public:
    FrameBuffer() = default;
    FrameBuffer(GLuint width, GLuint height);
    ~FrameBuffer();

    FrameBuffer(FrameBuffer const&) = delete;
    FrameBuffer& operator=(FrameBuffer const&) = delete;

    FrameBuffer(FrameBuffer&&) = delete;
    FrameBuffer& operator=(FrameBuffer&&) = delete;

    bool init(GLuint width, GLuint height);

    void bind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_);
    }

    static void bind_default()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    GLuint color_buffer(size_t n) const { assert(n < NumColorBuf); return color_buffers_[n]; }
    void use_color_buffer(size_t n) const { assert(n < NumColorBuf); glBindTexture(GL_TEXTURE_2D, color_buffers_[n]); }

    GLuint handle() const { return frame_buffer_; }

private:
    void cleanup();
    void create_texture_attachment(GLuint width, GLuint height);
    void create_render_buffer_attachement(GLuint width, GLuint height);

private:
    GLuint frame_buffer_;
    std::array<GLuint, NumColorBuf> color_buffers_;
    boost::optional<GLuint> render_buffer_;
};
