#pragma once

#include "md2view/gl/gl.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <gsl-lite/gsl-lite.hpp>
#include <spdlog/spdlog.h>

#include <array>
#include <cassert>
#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

class Shader {
public:
    Shader() = default;
    ~Shader();

    // non-copyable
    Shader(Shader const&) = delete;
    Shader& operator=(Shader const&) = delete;

    // moveable
    Shader(Shader&& rhs) noexcept;
    Shader& operator=(Shader&& rhs) noexcept;

    Shader(std::filesystem::path const& vertex,
           std::filesystem::path const& fragment,
           std::optional<std::filesystem::path> const& geometry = {});

    [[nodiscard]] GLuint program() const { return program_; }
    void use() const { glUseProgram(program()); }

    void set_uniform_block_binding(gsl_lite::not_null<char const*> block,
                                   GLuint binding_point) const;

    [[nodiscard]] GLint
    uniform_location(gsl_lite::not_null<GLchar const*> name) const;
    [[nodiscard]] GLint uniform_location(std::string const& name) const {
        return uniform_location(
            gsl_lite::not_null<GLchar const*>(name.c_str()));
    }

    // maybe cache some of these
    [[nodiscard]] GLint model_location() const {
        return uniform_location("model");
    }
    [[nodiscard]] GLint view_location() const {
        return uniform_location("view");
    }
    [[nodiscard]] GLint projection_location() const {
        return uniform_location("projection");
    }
    [[nodiscard]] GLint camera_position_location() const {
        return uniform_location("cameraPos");
    }

    // type specific implementations
    static void set_uniform(GLint location, glm::vec3 const& v);
    static void set_uniform(GLint location, glm::vec2 const& v);
    static void set_uniform(GLint location, glm::vec4 const& v);
    static void set_uniform(GLint location, GLboolean b);
    static void set_uniform(GLint location, glm::mat4 const& m);
    static void set_uniform(GLint location, GLint i);
    static void set_uniform(GLint location, GLuint i);
    static void set_uniform(GLint location, GLfloat f);

    template <typename T>
    void set_uniform(gsl_lite::not_null<GLchar const*> name, T const& t) {
        set_uniform(uniform_location(name), t);
    }

    template <typename T>
    void set_uniform(std::string const& name, T const& t) {
        set_uniform(uniform_location(name), t);
    }

    template <std::size_t N>
    void set_uniform(GLint location, std::array<GLint, N> const& a) {
        glUniform1iv(location, N, a.data());
    }

    template <std::size_t N>
    void set_uniform(GLint location, std::array<GLfloat, N> const& a) {
        glUniform1fv(location, N, a.data());
    }

    template <std::size_t N>
    void set_uniform(GLint location, std::array<glm::vec2, N> const& a) {
        glUniform2fv(location, N, &a[0][0]);
    }

    template <std::size_t N>
    void set_uniform(GLint location, std::array<glm::vec3, N> const& a) {
        glUniform3fv(location, N, &a[0][0]);
    }

    template <std::size_t N>
    void set_uniform(gsl_lite::not_null<GLchar const*> name,
                     std::array<glm::mat4, N> const& a) {
        // do we need to set each element indvidually? seems slow
        for (size_t i = 0; i < a.size(); ++i) {
            std::ostringstream oss;
            oss << name << "[" << std::to_string(i) << "]";
            std::string s = oss.str();
            set_uniform(s.c_str(), a[i]);
        }
    }

    // ergonomic helpers
    void set_model(glm::mat4 const& m) const {
        set_uniform(model_location(), m);
    }
    void set_view(glm::mat4 const& m) const { set_uniform(view_location(), m); }
    void set_projection(glm::mat4 const& m) const {
        set_uniform(projection_location(), m);
    }
    void set_view_position(glm::vec3 const& v) const {
        set_uniform(camera_position_location(), v);
    }

private:
    [[nodiscard]] bool
    init(std::filesystem::path const& vertex,
         std::filesystem::path const& fragment,
         std::optional<std::filesystem::path> const& geometry = {});
    [[nodiscard]] static bool compile_shader(GLenum shader_type,
                                             std::filesystem::path const& path,
                                             GLuint& handle);
    [[nodiscard]] static bool link_program(GLuint program);
    void cleanup();

    GLuint program_{};
};
