#include "frame_buffer.hpp"

template <size_t NumColorBuf, bool RenderBuf>
FrameBuffer<NumColorBuf, RenderBuf>::FrameBuffer(GLuint width, GLuint height)
{
    if (!init(width, height)) {
        throw std::runtime_error("failed to initalize framebuffer");
    }
}

template <size_t NumColorBuf, bool RenderBuf>
bool FrameBuffer<NumColorBuf, RenderBuf>::init(GLuint width, GLuint height)
{
    glCheckError();
    glGenFramebuffers(1, &frame_buffer_);
    glCheckError();
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_);
    glCheckError();

    create_texture_attachment(width, height);
    glCheckError();

    if (RenderBuf) {
        create_render_buffer_attachement(width, height);
    }

    glCheckError();

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        cleanup();
        bind_default();
        return false;
    }

    bind_default();
    return true;
}

template <size_t NumColorBuf, bool RenderBuf>
FrameBuffer<NumColorBuf, RenderBuf>::~FrameBuffer()
{
    cleanup();
}

template <size_t NumColorBuf, bool RenderBuf>
void FrameBuffer<NumColorBuf, RenderBuf>::cleanup()
{
    if (RenderBuf) {
        GLuint rb = *render_buffer_;
        glDeleteRenderbuffers(1, &rb);
    }

    glDeleteTextures(NumColorBuf, color_buffers_.data());
    glDeleteFramebuffers(1, &frame_buffer_);
}

template <size_t NumColorBuf, bool RenderBuf>
void FrameBuffer<NumColorBuf, RenderBuf>::create_texture_attachment(GLuint width, GLuint height)
{
    glGenTextures(NumColorBuf, color_buffers_.data());

    for (auto i = 0u; i < NumColorBuf; ++i) {
        glBindTexture(GL_TEXTURE_2D, color_buffers_[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, color_buffers_[i], 0);
    }

    glCheckError();
}

template <size_t NumColorBuf, bool RenderBuf>
void FrameBuffer<NumColorBuf, RenderBuf>::create_render_buffer_attachement(GLuint width, GLuint height)
{
    GLuint rb;
    glGenRenderbuffers(1, &rb);
    glBindRenderbuffer(GL_RENDERBUFFER, rb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rb);
    render_buffer_ = rb;
    glCheckError();
}
