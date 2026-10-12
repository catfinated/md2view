#include "md2view/gl/frame_buffer.hpp"
#include "md2view/gl/gl.hpp"

#include <gsl-lite/gsl-lite.hpp>

#include <cstddef>
#include <stdexcept>

namespace GL {

FrameBuffer::FrameBuffer(GLuint width,
                         GLuint height,
                         size_t numColorBuffers,
                         bool enableRenderBuffer)
    : colorBuffers_(numColorBuffers) {
    if (!init(width, height, enableRenderBuffer)) {
        throw std::runtime_error("failed to initalize framebuffer");
    }
}

bool FrameBuffer::init(GLuint width, GLuint height, bool enableRenderBuffer) {
    glCheckError();
    glGenFramebuffers(1, &frameBuffer_);
    glCheckError();
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer_);
    glCheckError();

    createTextureAttachment(width, height);
    glCheckError();

    if (enableRenderBuffer) {
        createRenderBufferAttachement(width, height);
    }

    glCheckError();

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        cleanup();
        bindDefault();
        return false;
    }

    bindDefault();
    return true;
}

FrameBuffer::~FrameBuffer() { cleanup(); }

void FrameBuffer::cleanup() {
    if (renderBuffer_) {
        GLuint rb = *renderBuffer_;
        glDeleteRenderbuffers(1, &rb);
    }

    auto const count = gsl_lite::narrow_cast<GLsizei>(colorBuffers_.size());
    glDeleteTextures(count, colorBuffers_.data());
    glDeleteFramebuffers(1, &frameBuffer_);
}

void FrameBuffer::createTextureAttachment(GLuint width, GLuint height) {
    auto const count = gsl_lite::narrow_cast<GLsizei>(colorBuffers_.size());
    glGenTextures(count, colorBuffers_.data());

    for (auto i = 0u; i < colorBuffers_.size(); ++i) {
        glBindTexture(GL_TEXTURE_2D, colorBuffers_[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                     gsl_lite::narrow_cast<GLsizei>(width),
                     gsl_lite::narrow_cast<GLsizei>(height), 0, GL_RGB,
                     GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
                               GL_TEXTURE_2D, colorBuffers_[i], 0);
    }

    glCheckError();
}

void FrameBuffer::createRenderBufferAttachement(GLuint width, GLuint height) {
    GLuint rb;
    glGenRenderbuffers(1, &rb);
    glBindRenderbuffer(GL_RENDERBUFFER, rb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
                          gsl_lite::narrow_cast<GLsizei>(width),
                          gsl_lite::narrow_cast<GLsizei>(height));
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, rb);
    renderBuffer_ = rb;
    glCheckError();
}

} // namespace GL
