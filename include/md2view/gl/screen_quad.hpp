#pragma once

#include "md2view/gl/gl.hpp"

#include <array>

class Shader;

namespace GL {

/// Full-screen quad for post-processing passes.
///
/// Owns a VAO and VBO covering the NDC clip-space rectangle [-1, 1] with UV
/// coordinates [0, 1]. Used to apply screen-space shaders (blur, glow
/// composite) over the scene framebuffer textures.
class ScreenQuad {
public:
    ScreenQuad();
    ~ScreenQuad();

    ScreenQuad(ScreenQuad const&) = delete;
    ScreenQuad& operator=(ScreenQuad const&) = delete;
    ScreenQuad(ScreenQuad&&) = delete;
    ScreenQuad& operator=(ScreenQuad&&) = delete;

    /// Bind the VAO and draw the quad as two triangles.
    void draw(Shader& shader) const;

private:
    GLuint vao_{};
    GLuint vbo_{};
    static std::array<GLfloat, 24> vertices_;
};

} // namespace GL
