#pragma once

#include "gl.hpp"

#include <array>

class Shader;

class ScreenQuad {
public:
    ScreenQuad();
    ~ScreenQuad();

    ScreenQuad(ScreenQuad const&) = delete;
    ScreenQuad& operator=(ScreenQuad const&) = delete;

    ScreenQuad(ScreenQuad&&) = delete;
    ScreenQuad& operator=(ScreenQuad&&) = delete;

    void draw(Shader& shader) const;

private:
    GLuint vao_{};
    GLuint vbo_{};
    static std::array<GLfloat, 24> vertices_;
};
