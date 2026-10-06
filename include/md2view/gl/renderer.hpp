#pragma once

#include "md2view/gl/gl.hpp"

#include "md2view/gl/frame_buffer.hpp"
#include "md2view/gl/resource_manager.hpp"
#include "md2view/gl/screen_quad.hpp"
#include "md2view/gl/shader.hpp"
#include "md2view/window.hpp"

#include <GLFW/glfw3.h>

#include <array>

namespace GL {

class Renderer {
public:
    static constexpr std::array kWindowHints{
        WindowHint{GLFW_CONTEXT_VERSION_MAJOR, 4},
        WindowHint{GLFW_CONTEXT_VERSION_MINOR, 1},
        WindowHint{GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE},
        WindowHint{GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE}};

    Renderer(Window& window, ResourceManager& resourceManager);

    Renderer(Renderer const&) = delete;
    Renderer& operator=(Renderer const&) = delete;
    Renderer(Renderer&&) noexcept = delete;
    Renderer& operator=(Renderer&&) noexcept = delete;

    [[nodiscard]] bool vsyncOn() const noexcept { return vsyncOn_; }
    void setVsyncOn(bool flag);

    [[nodiscard]] bool glowOn() const noexcept { return glowOn_; }
    void setGlowOn(bool flag) { glowOn_ = flag; }

    std::array<float, 4> const& clearColor() const noexcept {
        return clearColor_;
    }
    void setClearColor(std::array<float, 4> const& color);

    void beginFrame();
    void postProcessFrame();
    void endFrame();

    void onFramebufferResize(Window::Extent extent);
    [[nodiscard]] float aspectRatio() const;

private:
    Window& window_;
    Window::Extent fbSize_{};
    std::array<float, 4> clearColor_{0.2f, 0.2f, 0.2f, 1.0f};
    GLint disableBlurLoc_{};
    bool vsyncOn_{false};
    bool glowOn_{false};

    std::unique_ptr<GL::ScreenQuad> screenQuad_;
    std::unique_ptr<GL::FrameBuffer> blurFb_;
    std::unique_ptr<GL::FrameBuffer> mainFb_;
    std::shared_ptr<GL::Shader> blurShader_;
    std::shared_ptr<GL::Shader> glowShader_;
};

} // namespace GL
