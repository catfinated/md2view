#pragma once

#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <span>

/// Mesh identifier, 0 is invalid
struct MeshHandle {
    std::uint32_t id{};

    bool operator==(MeshHandle const&) const = default;
};

/// Texture identififer, 0 is invalid
struct TextureHandle {
    std::uint32_t id{};

    bool operator==(TextureHandle const&) const = default;
};

/// Render interface
class Renderer {
public:
    virtual ~Renderer() = default;

    Renderer(Renderer const&) = delete;
    Renderer& operator=(Renderer const&) = delete;
    Renderer(Renderer&&) noexcept = delete;
    Renderer& operator=(Renderer&&) noexcept = delete;

    [[nodiscard]] virtual bool vsyncOn() const noexcept = 0;
    virtual void setVsyncOn(bool flag) = 0;

    [[nodiscard]] virtual bool glowOn() const noexcept = 0;
    virtual void setGlowOn(bool flag) = 0;

    [[nodiscard]] virtual std::array<float, 4> const&
    clearColor() const noexcept = 0;
    virtual void setClearColor(std::array<float, 4> const& color) = 0;

    /// Framebuffer width divided by height
    [[nodiscard]] virtual float aspectRatio() const = 0;

    /// Set the camera for the current frame
    virtual void setCamera(glm::mat4 const& view,
                           glm::mat4 const& projection) = 0;

    /// Create a mesh from vertices and texture coordinates
    ///
    /// @param vertices  Flat array of vertex positions (3 per triangle)
    /// @param texcoords Texture coordinates, parallel to @p vertices
    [[nodiscard]] virtual MeshHandle
    createMesh(std::span<glm::vec3 const> vertices,
               std::span<glm::vec2 const> texcoords) = 0;

    /// Update a mesh's vertices
    /// @param mesh The handle to the mesh being updated
    /// @param vertices New vertices for mesh, must be the same size as when the
    /// mesh was created.
    virtual void updateMesh(MeshHandle mesh,
                            std::span<glm::vec3 const> vertices) = 0;

    /// Destroy a mesh
    virtual void destroyMesh(MeshHandle mesh) = 0;

protected:
    Renderer() = default;
};
