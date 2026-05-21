#pragma once

#include "md2view/gl/gl.hpp"

#include <glm/glm.hpp>

#include <array>
#include <span>

class Shader;

namespace GL {

// Owns GPU resources (VAO + VBOs) for one triangulated mesh.
// Vertex data is dynamic (updated each frame via sync()); texcoord data is
// static.
class Mesh {
public:
    Mesh(std::span<glm::vec3 const> vertices,
         std::span<glm::vec2 const> texcoords);
    ~Mesh();

    Mesh(Mesh const&) = delete;
    Mesh& operator=(Mesh const&) = delete;
    Mesh(Mesh&&) = delete;
    Mesh& operator=(Mesh&&) = delete;

    void sync(std::span<glm::vec3 const> vertices);
    void draw(Shader& shader) const;

private:
    GLuint vao_{};
    std::array<GLuint, 2> vbo_{};
    GLsizei vertex_count_{};
};

} // namespace GL
