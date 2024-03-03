#pragma once

#include "gl.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <spdlog/spdlog.h>

#include <array>
#include <string>
#include <sstream>
#include <cassert>

class Shader
{
public:
    Shader() = default;
    ~Shader();

    // non-copyable
    Shader(Shader const&) = delete;
    Shader& operator=(Shader const&) = delete;

    // moveable
    Shader(Shader&& rhs);
    Shader& operator=(Shader&& rhs);

    Shader(std::string const& vertex,
           std::string const& fragment,
           std::string const& geometry = std::string());

    bool init(std::string const& vertex,
              std::string const& fragment,
              std::string const& geometry = std::string());

    GLuint program() const { return program_; }
    void use() const { glUseProgram(program()); }
    bool initialized() const { return initialized_; }

    void set_uniform_block_binding(char const * block, GLuint binding_point);

    GLint uniform_location(GLchar const * name) const;
    GLint uniform_location(std::string const& name) const { return uniform_location(name.c_str()); }

    // maybe cache some of these
    GLint model_location() const { return uniform_location("model"); }
    GLint view_location() const { return uniform_location("view"); }
    GLint projection_location() const { return uniform_location("projection"); }
    GLint camera_position_location() const { return uniform_location("cameraPos"); }

    // type specific implementations
    void set_uniform(GLint location, glm::vec3 const& v);
    void set_uniform(GLint location, glm::vec2 const& v);
    void set_uniform(GLint location, glm::vec4 const& v);
    void set_uniform(GLint location, GLboolean b);
    void set_uniform(GLint location, glm::mat4 const& m);
    void set_uniform(GLint location, GLint i);
    void set_uniform(GLint location, GLuint i);
    void set_uniform(GLint location, GLfloat f);

    template <typename T>
    void set_uniform(GLchar const * name, T const& t)
    {
        set_uniform(uniform_location(name), t);
    }

    template <typename T>
    void set_uniform(std::string const& name, T const& t)
    {
        set_uniform(uniform_location(name), t);
    }

    template <std::size_t N>
    void set_uniform(GLint location, std::array<GLint, N> const& a)
    {
        glUniform1iv(location, N, a.data());
    }

    template <std::size_t N>
    void set_uniform(GLint location, std::array<GLfloat, N> const& a)
    {
        glUniform1fv(location, N, a.data());
    }

    template <std::size_t N>
    void set_uniform(GLint location, std::array<glm::vec2, N> const& a)
    {
        glUniform2fv(location, N, &a[0][0]);
    }

    template <std::size_t N>
    void set_uniform(GLint location, std::array<glm::vec3, N> const& a)
    {
        glUniform3fv(location, N, &a[0][0]);
    }

    template <std::size_t N>
    void set_uniform(GLchar const * name, std::array<glm::mat4, N> const& a)
    {
        assert(name);

        // do we need to set each element indvidually? seems slow
        for (size_t i = 0; i < a.size(); ++i) {
            std::ostringstream oss;
            oss << name << "[" << std::to_string(i) << "]";
            std::string s = oss.str();
            set_uniform(s.c_str(), a[i]);
        }
    }

    // ergonomic helpers
    void set_model(glm::mat4 const& m) { set_uniform(model_location(), m); }
    void set_view(glm::mat4 const& m) { set_uniform(view_location(), m); }
    void set_projection(glm::mat4 const& m) { set_uniform(projection_location(), m); }
    void set_view_position(glm::vec3 const& v) { set_uniform(camera_position_location(), v); }

private:
    bool compile_shader(GLenum shader_type, char const * path, GLuint& handle);
    bool link_program();
    void cleanup();

private:
    GLuint program_;
    bool initialized_ = false;
};

