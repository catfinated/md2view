#pragma once

#include "md2view/gl/gl.hpp"

#include <glm/glm.hpp>

#include <array>
#include <span>

class Shader;

namespace GL {

/// Model-agnostic GPU mesh owning a VAO and two VBOs.
///
/// Holds one vertex-position buffer (dynamic, updated every frame via `sync()`)
/// and one texture-coordinate buffer (static, uploaded once at construction).
/// The draw call issues a single `glDrawArrays(GL_TRIANGLES, ...)`.
///
/// This class is deliberately decoupled from any specific model type. Any
/// source that can produce a flat `span<glm::vec3>` of world-space vertex
/// positions and a `span<glm::vec2>` of texture coordinates is compatible.
///
/// Typical per-frame usage:
/// @code
/// model.update(dt);                          // advance animation
/// mesh.sync(model.interpolated_vertices());  // upload to GPU
/// mesh.draw(shader);                         // draw
/// @endcode
class Mesh {
public:
    /// Allocate GPU resources and upload initial vertex and texcoord data.
    ///
    /// @param vertices  Flat array of world-space vertex positions
    ///                  (num_triangles × 3 entries).
    /// @param texcoords Flat array of normalised texture coordinates,
    ///                  parallel to @p vertices.
    Mesh(std::span<glm::vec3 const> vertices,
         std::span<glm::vec2 const> texcoords);

    /// Release the VAO and VBOs.
    ~Mesh();

    Mesh(Mesh const&) = delete;
    Mesh& operator=(Mesh const&) = delete;
    Mesh(Mesh&&) = delete;
    Mesh& operator=(Mesh&&) = delete;

    /// Re-upload vertex positions after an animation update.
    ///
    /// Only the dynamic vertex buffer is updated; the static texcoord buffer
    /// is untouched. @p vertices must be the same size as the span passed to
    /// the constructor.
    void sync(std::span<glm::vec3 const> vertices);

    /// Bind the VAO and issue `glDrawArrays`.
    void draw(Shader& shader) const;

private:
    GLuint vao_{};
    std::array<GLuint, 2> vbo_{};
    GLsizei vertex_count_{};
};

} // namespace GL
