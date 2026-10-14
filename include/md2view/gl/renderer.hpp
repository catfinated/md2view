#pragma once

#include "md2view/gl/gl.hpp"

#include "md2view/gl/frame_buffer.hpp"
#include "md2view/gl/mesh.hpp"
#include "md2view/gl/screen_quad.hpp"
#include "md2view/gl/shader.hpp"
#include "md2view/gl/texture2d.hpp"
#include "md2view/renderer.hpp"
#include "md2view/window.hpp"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <unordered_map>

namespace GL {

class Renderer final : public ::Renderer {
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

    /// @param shadersDir Directory containing GLSL shader sources
    Renderer(Window& window, std::filesystem::path const& shadersDir);
    ~Renderer() override = default;

    Renderer(Renderer const&) = delete;
    Renderer& operator=(Renderer const&) = delete;
    Renderer(Renderer&&) noexcept = delete;
    Renderer& operator=(Renderer&&) noexcept = delete;

    [[nodiscard]] bool vsyncOn() const noexcept override { return vsyncOn_; }
    void setVsyncOn(bool flag) override;

    [[nodiscard]] bool glowOn() const noexcept override { return glowOn_; }
    void setGlowOn(bool flag) override { glowOn_ = flag; }

    [[nodiscard]] std::array<float, 4> const&
    clearColor() const noexcept override {
        return clearColor_;
    }
    void setClearColor(std::array<float, 4> const& color) override;

    void beginFrame();
    void postProcessFrame();
    void endFrame();

    /// Set the camera for the current frame
    void setCamera(glm::mat4 const& view, glm::mat4 const& projection) override;

    [[nodiscard]] MeshHandle
    createMesh(std::span<glm::vec3 const> vertices,
               std::span<glm::vec2 const> texcoords) override;
    void updateMesh(MeshHandle mesh,
                    std::span<glm::vec3 const> vertices) override;
    void destroyMesh(MeshHandle mesh) override;

    [[nodiscard]] TextureHandle createTexture(Image const& image) override;
    void destroyTexture(TextureHandle texture) override;
    [[nodiscard]] glm::uvec2 textureSize(TextureHandle texture) const override;
    [[nodiscard]] std::uint64_t
    imguiTexture(TextureHandle texture) const override;

    void drawModel(MeshHandle mesh,
                   TextureHandle skin,
                   glm::mat4 const& model,
                   glm::vec3 const& glowColor) override;

    void onFramebufferResize(Window::Extent extent);
    [[nodiscard]] float aspectRatio() const override;

private:
    [[nodiscard]] Texture2D const& texture(TextureHandle texture) const;

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
    GL::Shader blurShader_;
    GL::Shader glowShader_;
    GL::Shader modelShader_;

    std::unordered_map<std::uint32_t, std::unique_ptr<Mesh>> meshes_;
    std::uint32_t nextMeshId_{1};
    std::unordered_map<std::uint32_t, Texture2D> textures_;
    std::uint32_t nextTextureId_{1};
};

} // namespace GL
