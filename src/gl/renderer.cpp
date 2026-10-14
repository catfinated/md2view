#include "md2view/gl/renderer.hpp"
#include "md2view/gl/frame_buffer.hpp"
#include "md2view/gl/gl.hpp"
#include "md2view/gl/mesh.hpp"
#include "md2view/gl/screen_quad.hpp"
#include "md2view/gl/shader.hpp"
#include "md2view/gl/texture2d.hpp"
#include "md2view/window.hpp"

#include <GLFW/glfw3.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

#include <array>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string_view>

namespace GL {

namespace {

/// Load `vertex.vert` and `fragment.frag` from @p dir
Shader loadShader(std::filesystem::path const& dir,
                  std::string_view vertex,
                  std::string_view fragment) {
    return Shader{dir / fmt::format("{}.vert", vertex),
                  dir / fmt::format("{}.frag", fragment)};
}

} // namespace

Renderer::Renderer(Window& window, std::filesystem::path const& shadersDir)
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

    blurShader_ = loadShader(shadersDir, "screen", "blur");
    blurShader_.use();
    disableBlurLoc_ = blurShader_.uniformLocation("disable_blur");
    GL::Shader::setUniform(disableBlurLoc_, 1);

    glowShader_ = loadShader(shadersDir, "screen", "glow");
    glowShader_.use();

    auto loc = glowShader_.uniformLocation("screenTexture");
    GL::Shader::setUniform(loc, 0);
    loc = glowShader_.uniformLocation("prepassTexture");
    GL::Shader::setUniform(loc, 1);
    loc = glowShader_.uniformLocation("blurredTexture");
    GL::Shader::setUniform(loc, 2);

    modelShader_ = loadShader(shadersDir, "md2", "md2");
    glowColorLoc_ = modelShader_.uniformLocation("glow_color");

    mainFb_->bind();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);
    // glDisable(GL_BLEND);
    glEnable(GL_BLEND);
    GL::FrameBuffer::bindDefault();

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
    std::array<GLenum, 2> drawBuffers{
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
    };
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
        blurShader_.use();
        GL::Shader::setUniform(disableBlurLoc_, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mainFb_->colorBuffer(1));
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        screenQuad_->draw(glowShader_);

        GL::FrameBuffer::bindDefault();
        glowShader_.use();
        glClear(GL_COLOR_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mainFb_->colorBuffer(0));
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, mainFb_->colorBuffer(1));
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, blurFb_->colorBuffer(0));
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        screenQuad_->draw(glowShader_);
    } else {
        GL::FrameBuffer::bindDefault();
        blurShader_.use();
        GL::Shader::setUniform(disableBlurLoc_, 1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mainFb_->colorBuffer(0));
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        screenQuad_->draw(blurShader_);
    }

    glCheckError();
}

void Renderer::endFrame() { glfwSwapBuffers(window_.get()); }

void Renderer::setCamera(glm::mat4 const& view, glm::mat4 const& projection) {
    modelShader_.use();
    modelShader_.setView(view);
    modelShader_.setProjection(projection);
}

void Renderer::drawModel(Mesh const& mesh,
                         Texture2D const& skin,
                         glm::mat4 const& model,
                         glm::vec3 const& glowColor) {
    modelShader_.use();
    modelShader_.setModel(model);
    GL::Shader::setUniform(glowColorLoc_, glowColor);
    glActiveTexture(GL_TEXTURE0);
    skin.bind();
    mesh.draw(modelShader_);
    glCheckError();
}

void Renderer::onFramebufferResize(Window::Extent extent) {
    fbSize_ = extent;
    GL::FrameBuffer::bindDefault();
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
