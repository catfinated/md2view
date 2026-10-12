#pragma once

#include "md2view/gl/gl.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <gsl-lite/gsl-lite.hpp>

#include <array>
#include <cassert>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <sstream>
#include <string>

namespace GL {

/// OpenGL shader program (vertex + optional geometry + fragment stages).
///
/// Loads GLSL source from files, compiles each stage, and links the program.
/// Provides typed `set_uniform()` overloads for common GLM types and a set of
/// named convenience helpers (`set_model()`, `set_view()`, etc.) for the
/// standard MVP uniforms.
class Shader {
public:
    Shader() = default;
    ~Shader();

    Shader(Shader const&) = delete;
    Shader& operator=(Shader const&) = delete;

    Shader(Shader&& rhs) noexcept;
    Shader& operator=(Shader&& rhs) noexcept;

    /// Compile and link a shader program from source files.
    ///
    /// @param vertex   Path to the GLSL vertex shader source.
    /// @param fragment Path to the GLSL fragment shader source.
    /// @param geometry Optional path to a GLSL geometry shader source.
    /// @throws std::runtime_error if any stage fails to compile or the program
    ///         fails to link.
    Shader(std::filesystem::path const& vertex,
           std::filesystem::path const& fragment,
           std::optional<std::filesystem::path> const& geometry = {});

    /// The linked OpenGL program handle.
    [[nodiscard]] GLuint program() const { return program_; }

    /// Install this program as the current OpenGL pipeline program.
    void use() const { glUseProgram(program()); }

    /// Bind a named uniform block to a binding point.
    void setUniformBlockBinding(gsl_lite::not_null<char const*> block,
                                GLuint bindingPoint) const;

    /// @name Uniform location lookup
    /// @{
    [[nodiscard]] GLint
    uniformLocation(gsl_lite::not_null<GLchar const*> name) const;
    [[nodiscard]] GLint uniformLocation(GLchar const* name) const {
        return uniformLocation(gsl_lite::not_null<GLchar const*>(name));
    }
    [[nodiscard]] GLint uniformLocation(std::string const& name) const {
        return uniformLocation(gsl_lite::not_null<GLchar const*>(name.c_str()));
    }
    /// @}

    /// @name Standard MVP uniform locations
    /// @{
    [[nodiscard]] GLint modelLocation() const {
        return uniformLocation("model");
    }
    [[nodiscard]] GLint viewLocation() const { return uniformLocation("view"); }
    [[nodiscard]] GLint projectionLocation() const {
        return uniformLocation("projection");
    }
    [[nodiscard]] GLint cameraPositionLocation() const {
        return uniformLocation("cameraPos");
    }
    /// @}

    /// @name Typed uniform setters (static — call after `use()`)
    /// @{
    static void setUniform(GLint location, glm::vec2 const& v);
    static void setUniform(GLint location, glm::vec3 const& v);
    static void setUniform(GLint location, glm::vec4 const& v);
    static void setUniform(GLint location, glm::mat4 const& m);
    static void setUniform(GLint location, GLboolean b);
    static void setUniform(GLint location, GLint i);
    static void setUniform(GLint location, GLuint i);
    static void setUniform(GLint location, GLfloat f);
    /// @}

    template <typename T>
    void setUniform(gsl_lite::not_null<GLchar const*> name, T const& t) {
        set_uniform(uniformLocation(name), t);
    }

    template <typename T> void setUniform(std::string const& name, T const& t) {
        set_uniform(uniformLocation(name), t);
    }

    template <std::size_t N>
    void setUniform(GLint location, std::array<GLint, N> const& a) {
        glUniform1iv(location, N, a.data());
    }

    template <std::size_t N>
    void setUniform(GLint location, std::array<GLfloat, N> const& a) {
        glUniform1fv(location, N, a.data());
    }

    template <std::size_t N>
    void setUniform(GLint location, std::array<glm::vec2, N> const& a) {
        glUniform2fv(location, N, &a[0][0]);
    }

    template <std::size_t N>
    void setUniform(GLint location, std::array<glm::vec3, N> const& a) {
        glUniform3fv(location, N, &a[0][0]);
    }

    template <std::size_t N>
    void setUniform(gsl_lite::not_null<GLchar const*> name,
                    std::array<glm::mat4, N> const& a) {
        for (size_t i = 0; i < a.size(); ++i) {
            std::ostringstream oss;
            oss << name << "[" << std::to_string(i) << "]";
            std::string s = oss.str();
            set_uniform(s.c_str(), a[i]);
        }
    }

    /// @name MVP convenience setters
    /// @{
    void setModel(glm::mat4 const& m) const { setUniform(modelLocation(), m); }
    void setView(glm::mat4 const& m) const { setUniform(viewLocation(), m); }
    void setProjection(glm::mat4 const& m) const {
        setUniform(projectionLocation(), m);
    }
    void setViewPosition(glm::vec3 const& v) const {
        setUniform(cameraPositionLocation(), v);
    }
    /// @}

private:
    [[nodiscard]] bool
    init(std::filesystem::path const& vertex,
         std::filesystem::path const& fragment,
         std::optional<std::filesystem::path> const& geometry = {});
    [[nodiscard]] static bool compileShader(GLenum shaderType,
                                            std::filesystem::path const& path,
                                            GLuint& handle);
    [[nodiscard]] static bool linkProgram(GLuint program);
    void cleanup();

    GLuint program_{};
};

} // namespace GL
