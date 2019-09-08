#pragma once

#include "gl.hpp"
#include "shader.hpp"

#include <array>

class ScreenQuad
{
public:
    ScreenQuad() = default;
    ~ScreenQuad();

    ScreenQuad(ScreenQuad const&) = delete;
    ScreenQuad& operator=(ScreenQuad const&) = delete;

    ScreenQuad(ScreenQuad&&) = delete;
    ScreenQuad& operator=(ScreenQuad&&) = delete;

    void init();
    void draw(Shader& shader);

private:
    GLuint vao_;
    GLuint vbo_;
    bool   initialized_ = false;
    static std::array<GLfloat, 24> vertices_;
};

inline void ScreenQuad::init()
{
    if (initialized_) { return; }

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_), vertices_.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), GL_BUFFER_OFFSET(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), GL_BUFFER_OFFSET(2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    glCheckError();
}

inline ScreenQuad::~ScreenQuad()
{
    if (initialized_) {
        glDeleteVertexArrays(1, &vao_);
        glDeleteBuffers(1, &vbo_);
    }
}

inline void ScreenQuad::draw(Shader& shader)
{
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}
