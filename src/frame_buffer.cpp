#include "md2view/frame_buffer.hpp"

FrameBuffer::FrameBuffer(GLuint width,
                         GLuint height,
                         size_t num_color_buffers,
                         bool enable_render_buffer)
    : color_buffers_(num_color_buffers) {
    if (!init(width, height, enable_render_buffer)) {
        throw std::runtime_error("failed to initalize framebuffer");
    }
}

bool FrameBuffer::init(GLuint width, GLuint height, bool enable_render_buffer) {
    glCheckError();
    glGenFramebuffers(1, &frame_buffer_);
    glCheckError();
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_);
    glCheckError();

    create_texture_attachment(width, height);
    glCheckError();

    if (enable_render_buffer) {
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

FrameBuffer::~FrameBuffer() { cleanup(); }

void FrameBuffer::cleanup() {
    if (render_buffer_) {
        GLuint rb = *render_buffer_;
        glDeleteRenderbuffers(1, &rb);
    }

    glDeleteTextures(color_buffers_.size(), color_buffers_.data());
    glDeleteFramebuffers(1, &frame_buffer_);
}

void FrameBuffer::create_texture_attachment(GLuint width, GLuint height) {
    glGenTextures(color_buffers_.size(), color_buffers_.data());

    for (auto i = 0u; i < color_buffers_.size(); ++i) {
        glBindTexture(GL_TEXTURE_2D, color_buffers_[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                     GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
                               GL_TEXTURE_2D, color_buffers_[i], 0);
    }

    glCheckError();
}

void FrameBuffer::create_render_buffer_attachement(GLuint width,
                                                   GLuint height) {
    GLuint rb;
    glGenRenderbuffers(1, &rb);
    glBindRenderbuffer(GL_RENDERBUFFER, rb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, rb);
    render_buffer_ = rb;
    glCheckError();
}
