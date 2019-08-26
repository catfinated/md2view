#pragma once

#include "texture.hpp"
#include "shader.hpp"
#include "texture2D.hpp"
#include "resource_manager.hpp"

#ifndef BUFFER_OFFSET
#define BUFFER_OFFSET(idx) (static_cast<char*>(0) + (idx))
#endif

class Quad
{
public:
    using vertices_t = std::array<GLfloat, 30>;

    Quad();
    virtual ~Quad();
    virtual void Draw(blue::Shader& shader);

    vertices_t const& vertices() const { return vertices_; }

    static vertices_t const& default_vertices();

private:
    GLuint VAO_;
    GLuint VBO_;
    vertices_t vertices_;
};

class TexturedQuad : public Quad
{
public:
    TexturedQuad(std::string const& texture, bool alpha = false);
    virtual void Draw(blue::Shader& shader);

private:
    //GLint textureID_;
    blue::Texture2D * texture_;

};

class ScreenQuad
{
public:
    using vertices_t = std::array<GLfloat, 24>;

    ScreenQuad();
    virtual ~ScreenQuad();
    virtual void Draw(blue::Shader& shader);

    vertices_t const& vertices() const { return vertices_; }

    static vertices_t const& default_vertices();

private:
    GLuint VAO_;
    GLuint VBO_;
    vertices_t vertices_;
};

inline
Quad::Quad()
    : vertices_(default_vertices())
{
    glGenVertexArrays(1, &VAO_);
    glGenBuffers(1, &VBO_);

    glBindVertexArray(VAO_);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices()), vertices().data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), BUFFER_OFFSET(0));
    glEnableVertexAttribArray(0);
    //glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), BUFFER_OFFSET(3 * sizeof(GLfloat)));
    //glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), BUFFER_OFFSET(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    CheckError();
}

inline
Quad::~Quad()
{
    glDeleteVertexArrays(1, &VAO_);
    glDeleteBuffers(1, &VBO_);
}

inline
void Quad::Draw(blue::Shader& shader)
{
    glBindVertexArray(VAO_);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    CheckError();
}

inline
TexturedQuad::TexturedQuad(std::string const& texture, bool alpha)
    : Quad()
{
    static blue::ResourceManager rm("../src");
    auto& tex = rm.load_texture2D(texture.c_str(), "foo", alpha);
    texture_ = std::addressof(tex);
        //textureID_ = loadTexture(texture.c_str(), alpha);
}

inline
void TexturedQuad::Draw(blue::Shader& shader)
{
    //glActiveTexture(GL_TEXTURE0);
    //glBindTexture(GL_TEXTURE_2D, textureID_);
    texture_->bind();
    Quad::Draw(shader);
}

inline
ScreenQuad::ScreenQuad()
    : vertices_(default_vertices())
{
    glGenVertexArrays(1, &VAO_);
    glGenBuffers(1, &VBO_);

    glBindVertexArray(VAO_);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices()), vertices().data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), BUFFER_OFFSET(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), BUFFER_OFFSET(2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    CheckError();
}

inline
ScreenQuad::~ScreenQuad()
{
    glDeleteVertexArrays(1, &VAO_);
    glDeleteBuffers(1, &VBO_);
}

inline
void ScreenQuad::Draw(blue::Shader& shader)
{
    glBindVertexArray(VAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    //CheckError();
}

Quad::vertices_t const& Quad::default_vertices()
{
    static_assert(sizeof(vertices_t) == sizeof(GLfloat[30]), "sizeof");

    static vertices_t const v = {{
        // Positions         // Texture Coords (swapped y coordinates because texture is flipped upside down)
        0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
        0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
        1.0f, -0.5f,  0.0f,  1.0f,  1.0f,

        0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
        1.0f, -0.5f,  0.0f,  1.0f,  1.0f,
        1.0f,  0.5f,  0.0f,  1.0f,  0.0f
    }};

    return v;
}

ScreenQuad::vertices_t const& ScreenQuad::default_vertices()
{
    static_assert(sizeof(vertices_t) == sizeof(GLfloat[24]), "sizeof");

    static vertices_t const v = {{
       // Positions   // TexCoords
       -1.0f,  1.0f,  0.0f, 1.0f,
       -1.0f, -1.0f,  0.0f, 0.0f,
        1.0f, -1.0f,  1.0f, 0.0f,

       -1.0f,  1.0f,  0.0f, 1.0f,
        1.0f, -1.0f,  1.0f, 0.0f,
        1.0f,  1.0f,  1.0f, 1.0f
    }};

    return v;
}
