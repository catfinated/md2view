#pragma once

#include "md2view/gl/gl.hpp"

#include "md2view/gl/frame_buffer.hpp"
#include "md2view/gl/mesh.hpp"
#include "md2view/gl/resource_manager.hpp"
#include "md2view/gl/screen_quad.hpp"
#include "md2view/gl/shader.hpp"
#include "md2view/gl/texture2d.hpp"
#include "md2view/window.hpp"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <array>
#include <memory>

namespace GL {

class Renderer {
public:
    static constexpr std::array kWindowHints{
        WindowHint{.hint = GLFW_CONTEXT_VERSION_MAJOR, .value = 4},
        WindowHint{.hint = GLFW_CONTEXT_VERSION_MINOR, .value = 1},
        WindowHint{
            .hint = GLFW_OPENGL_PROFILE,
            .value = GLFW_OPENGL_CORE_PROFILE,
        },
        WindowHint{.hint = GLFW_OPENGL_FORWARD_COMPAT, .value = GLFW_TRUE},
    };

    Renderer(Window& window, ResourceManager& resourceManager);
    ~Renderer() = default;

    Renderer(Renderer const&) = delete;
    Renderer& operator=(Renderer const&) = delete;
    Renderer(Renderer&&) noexcept = delete;
    Renderer& operator=(Renderer&&) noexcept = delete;

    [[nodiscard]] bool vsyncOn() const noexcept { return vsyncOn_; }
    void setVsyncOn(bool flag);

    [[nodiscard]] bool glowOn() const noexcept { return glowOn_; }
    void setGlowOn(bool flag) { glowOn_ = flag; }

    [[nodiscard]] std::array<float, 4> const& clearColor() const noexcept {
        return clearColor_;
    }
    void setClearColor(std::array<float, 4> const& color);

    void beginFrame();
    void postProcessFrame();
    void endFrame();

    /// Set the camera for the current frame
    void setCamera(glm::mat4 const& view, glm::mat4 const& projection);

    /// Draw a textured model into the current frame
    ///
    /// @param glowColor Color written to the glow buffer, used when glow is on
    void drawModel(Mesh const& mesh,
                   Texture2D const& skin,
                   glm::mat4 const& model,
                   glm::vec3 const& glowColor);

    void onFramebufferResize(Window::Extent extent);
    [[nodiscard]] float aspectRatio() const;

private:
    Window& window_;
    Window::Extent fbSize_{};
    std::array<float, 4> clearColor_{0.2f, 0.2f, 0.2f, 1.0f};
    GLint disableBlurLoc_{};
    GLint glowColorLoc_{};
    bool vsyncOn_{false};
    bool glowOn_{false};

    std::unique_ptr<GL::ScreenQuad> screenQuad_;
    std::unique_ptr<GL::FrameBuffer> blurFb_;
    std::unique_ptr<GL::FrameBuffer> mainFb_;
    std::shared_ptr<GL::Shader> blurShader_;
    std::shared_ptr<GL::Shader> glowShader_;
    std::shared_ptr<GL::Shader> modelShader_;
};

} // namespace GL
