#include "md2view/gl/renderer.hpp"

#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <stdexcept>

namespace GL {

Renderer::Renderer(Window& window, ResourceManager& resourceManager)
    : window_(window) {

    glfwMakeContextCurrent(window_.get());

    spdlog::info("gl version: {}", glStrView(glGetString(GL_VERSION)));
    spdlog::info("gl renderer: {}", glStrView(glGetString(GL_RENDERER)));

    glewExperimental = GL_TRUE;
    auto const result = glewInit();

    // https://github.com/nigels-com/glew/issues/417
    if (result != GLEW_OK && result != GLEW_ERROR_NO_GLX_DISPLAY) {
        std::string_view err = glStrView(glewGetErrorString(result));
        throw std::runtime_error(fmt::format("failed to init glew: '{}'", err));
    }

    glGetError(); // glewInit is known to cause invalid enum error
    fbSize_ = window_.framebufferSize();
    glViewport(0, 0, fbSize_.width, fbSize_.height);
    spdlog::info("Default frame buffer size {}x{}", fbSize_.width,
                 fbSize_.height);

    GLint nrAttributes;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);
    spdlog::info("Maximum # of vertex attributes supported: {}", nrAttributes);

    setClearColor(clearColor_);
    blurFb_ = std::make_unique<GL::FrameBuffer>(fbSize_.width, fbSize_.height,
                                                1, false);
    mainFb_ = std::make_unique<GL::FrameBuffer>(fbSize_.width, fbSize_.height,
                                                2, true);
    screenQuad_ = std::make_unique<GL::ScreenQuad>();

    blurShader_ = resourceManager.load_shader("blur", "screen");
    blurShader_->use();
    disableBlurLoc_ = blurShader_->uniform_location("disable_blur");
    GL::Shader::set_uniform(disableBlurLoc_, 1);

    glowShader_ = resourceManager.load_shader("glow", "screen");
    glowShader_->use();

    auto loc = glowShader_->uniform_location("screenTexture");
    GL::Shader::set_uniform(loc, 0);
    loc = glowShader_->uniform_location("prepassTexture");
    GL::Shader::set_uniform(loc, 1);
    loc = glowShader_->uniform_location("blurredTexture");
    GL::Shader::set_uniform(loc, 2);

    mainFb_->bind();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);
    // glDisable(GL_BLEND);
    glEnable(GL_BLEND);
    GL::FrameBuffer::bind_default();

    setVsyncOn(true);
    glCheckError();
}

void Renderer::setVsyncOn(bool flag) {
    if (vsyncOn_ != flag) {
        vsyncOn_ = flag;
        glfwSwapInterval(vsyncOn_ ? 1 : 0);
    }
}

void Renderer::setClearColor(std::array<float, 4> const& color) {
    clearColor_ = color;
    glClearColor(clearColor_[0], clearColor_[1], clearColor_[2], 1.0f);
}

void Renderer::beginFrame() {
    mainFb_->bind();
    std::array<GLenum, 2> drawBuffers{GL_COLOR_ATTACHMENT0,
                                      GL_COLOR_ATTACHMENT1};
    glDrawBuffers(drawBuffers.size(), drawBuffers.data());
    glCheckError();

    glClearBufferfv(GL_COLOR, 0, clearColor_.data());
    static const std::array<float, 4> black{0.0f, 0.0f, 0.0f, 0.0f};
    glClearBufferfv(GL_COLOR, 1, black.data());
    glCheckError();

    glActiveTexture(GL_TEXTURE0);
    glClear(GL_DEPTH_BUFFER_BIT);
    glCheckError();
}

void Renderer::postProcessFrame() {

    if (glowOn_) {
        // blur solid image
        blurFb_->bind();
        blurShader_->use();
        GL::Shader::set_uniform(disableBlurLoc_, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mainFb_->color_buffer(1));
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        screenQuad_->draw(*glowShader_);

        GL::FrameBuffer::bind_default();
        glowShader_->use();
        glClear(GL_COLOR_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mainFb_->color_buffer(0));
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, mainFb_->color_buffer(1));
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, blurFb_->color_buffer(0));
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        screenQuad_->draw(*glowShader_);
    } else {
        GL::FrameBuffer::bind_default();
        blurShader_->use();
        GL::Shader::set_uniform(disableBlurLoc_, 1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mainFb_->color_buffer(0));
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        screenQuad_->draw(*blurShader_);
    }

    glCheckError();
}

void Renderer::endFrame() { glfwSwapBuffers(window_.get()); }

void Renderer::onFramebufferResize(Window::Extent extent) {
    fbSize_ = extent;
    GL::FrameBuffer::bind_default();
    glViewport(0, 0, extent.width, extent.height);
    mainFb_ =
        std::make_unique<GL::FrameBuffer>(extent.width, extent.height, 2, true);
    blurFb_ = std::make_unique<GL::FrameBuffer>(extent.width, extent.height, 1,
                                                false);
}

float Renderer::aspectRatio() const {
    return static_cast<float>(fbSize_.width) /
           static_cast<float>(fbSize_.height);
}
} // namespace GL
