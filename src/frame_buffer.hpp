#pragma once

#include "gl.hpp"

#include <boost/optional.hpp>

#include <stdexcept>

template <size_t N = 1>
class FrameBuffer
{
public:
    FrameBuffer() = default;
    FrameBuffer(GLuint width, GLuint height, bool color_buffer_only = false);
    ~FrameBuffer();

    FrameBuffer(FrameBuffer const&) = delete;
    FrameBuffer& operator=(FrameBuffer const&) = delete;

    FrameBuffer(FrameBuffer&&) = delete;
    FrameBuffer& operator=(FrameBuffer&&) = delete;

    bool init(GLuint width, GLuint height, bool color_buffer_only = false);

    void bind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_);
    }

    static void bind_default()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    GLuint color_buffer(size_t n) const { assert(n < N); return color_buffers_[n]; }
    void use_color_buffer(size_t n) const { assert(n < N); glBindTexture(GL_TEXTURE_2D, color_buffers_[n]); }

    GLuint handle() const { return frame_buffer_; }

private:
    void cleanup();
    void create_texture_attachment(GLuint width, GLuint height);
    void create_render_buffer_attachement(GLuint width, GLuint height);

private:
    GLuint frame_buffer_;
    std::array<GLuint, N> color_buffers_;
    boost::optional<GLuint> render_buffer_;
};

template <size_t N>
FrameBuffer<N>::FrameBuffer(GLuint width, GLuint height, bool color_buffer_only)
{
    if (!init(width, height, color_buffer_only)) {
        throw std::runtime_error("failed to initalize framebuffer");
    }
}

template <size_t N>
bool FrameBuffer<N>::init(GLuint width, GLuint height, bool color_buffer_only)
{
    glCheckError();
    std::cout << "fb init\n";
    glGenFramebuffers(1, &frame_buffer_);
    glCheckError();
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_);
    glCheckError();

    std::cout << "fb init texture\n";
    create_texture_attachment(width, height);
    glCheckError();

    if (!color_buffer_only) {
        create_render_buffer_attachement(width, height);
    }

    glCheckError();

    std::cout << "fb check status\n";
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        cleanup();
        bind_default();
        return false;
    }

    bind_default();
    return true;
}

template <size_t N>
FrameBuffer<N>::~FrameBuffer()
{
    cleanup();
}

template <size_t N>
void FrameBuffer<N>::cleanup()
{
    if (render_buffer_) {
        GLuint rb = *render_buffer_;
        glDeleteRenderbuffers(1, &rb);
    }

    glDeleteTextures(N, color_buffers_.data());
    glDeleteFramebuffers(1, &frame_buffer_);
}

template <size_t N>
void FrameBuffer<N>::create_texture_attachment(GLuint width, GLuint height)
{
    glGenTextures(N, color_buffers_.data());

    for (auto i = 0; i < N; ++i) {
        glBindTexture(GL_TEXTURE_2D, color_buffers_[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, color_buffers_[i], 0);
    }

    glCheckError();
}

template <size_t N>
void FrameBuffer<N>::create_render_buffer_attachement(GLuint width, GLuint height)
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
