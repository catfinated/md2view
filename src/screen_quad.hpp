#pragma once

#include "gl.hpp"

#include <array>

class Shader;

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

