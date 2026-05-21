#include "md2view/gl/mesh.hpp"
#include "md2view/gl/shader.hpp"

#include <gsl-lite/gsl-lite.hpp>
#include <spdlog/spdlog.h>

namespace GL {

Mesh::Mesh(std::span<glm::vec3 const> vertices,
           std::span<glm::vec2 const> texcoords) {
    vertex_count_ = gsl_lite::narrow_cast<GLsizei>(vertices.size());

    glGenVertexArrays(1, &vao_);
    glGenBuffers(2, vbo_.data());

    glBindVertexArray(vao_);
    spdlog::debug("GL::Mesh vao={} vbo={},{}", vao_, vbo_[0], vbo_[1]);

    static_assert(sizeof(glm::vec3) == 12, "bad vec3 size");

    glBindBuffer(GL_ARRAY_BUFFER, vbo_[0]);
    glBufferData(
        GL_ARRAY_BUFFER,
        gsl_lite::narrow_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3)),
        vertices.data(), GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    static_assert(sizeof(glm::vec2) == 8, "bad vec2 size");

    glBindBuffer(GL_ARRAY_BUFFER, vbo_[1]);
    glBufferData(
        GL_ARRAY_BUFFER,
        gsl_lite::narrow_cast<GLsizeiptr>(texcoords.size() * sizeof(glm::vec2)),
        texcoords.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindVertexArray(0);

    glCheckError();
}

Mesh::~Mesh() {
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(2, vbo_.data());
}

void Mesh::sync(std::span<glm::vec3 const> vertices) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_[0]);
    glBufferSubData(
        GL_ARRAY_BUFFER, 0,
        gsl_lite::narrow_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3)),
        vertices.data());
}

void Mesh::draw(Shader& /* shader */) const {
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, vertex_count_);
    glCheckError();
}

} // namespace GL
