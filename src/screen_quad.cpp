#include "md2view/screen_quad.hpp"
#include "md2view/shader.hpp"

std::array<GLfloat, 24> ScreenQuad::vertices_ = {
    {// Positions   // TexCoords
     -1.0f, 1.0f, 0.0f, 1.0f,  -1.0f, -1.0f,
     0.0f,  0.0f, 1.0f, -1.0f, 1.0f,  0.0f,

     -1.0f, 1.0f, 0.0f, 1.0f,  1.0f,  -1.0f,
     1.0f,  0.0f, 1.0f, 1.0f,  1.0f,  1.0f}};

ScreenQuad::ScreenQuad() {
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_), vertices_.data(),
                 GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
                          GL_BUFFER_OFFSET(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
                          GL_BUFFER_OFFSET(2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    glCheckError();
}

ScreenQuad::~ScreenQuad() {
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
}

void ScreenQuad::draw(Shader& /* shader */) {
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}
