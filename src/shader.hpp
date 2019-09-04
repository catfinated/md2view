#pragma once

#include "gl.hpp"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <array>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cassert>
#include <stdexcept>

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
            //std::cout << s << ' ' << uniform_location(s.c_str()) << '\n';
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

inline Shader::Shader(std::string const& vertex,
                      std::string const& fragment,
                      std::string const& geometry)
    : program_(0)
{
    if (!init(vertex, fragment, geometry)) {
        throw std::runtime_error("shader initialization failed");
    }
}

inline Shader::~Shader()
{
    cleanup();
}

inline Shader::Shader(Shader&& rhs)
{
    program_ = rhs.program_;
    initialized_ = rhs.initialized_;

    rhs.program_ = 0;
    rhs.initialized_ = false;
}

inline Shader& Shader::operator=(Shader&& rhs)
{
    if (this != &rhs) {
        cleanup();
        program_ = rhs.program_;
        initialized_ = rhs.initialized_;

        rhs.program_ = 0;
        rhs.initialized_ = false;
    }

    return *this;
}

inline void Shader::cleanup()
{
    if (initialized_) {
        glDeleteProgram(program_);
        initialized_ = false;
    }
}

inline bool Shader::init(std::string const& vertex,
                         std::string const& fragment,
                         std::string const& geometry)
{
    if (vertex.empty()) {
        std::cerr << "shader vertex path cannot be empty" << '\n';
        return false;
    }

    if (fragment.empty()) {
        std::cerr << "shader fragment path cannot be empty" << '\n';
        return false;
    }

    program_ = glCreateProgram();

    GLuint vertex_id, fragment_id, geometry_id;

    if (!compile_shader(GL_VERTEX_SHADER, vertex.c_str(), vertex_id)) {
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n";
        return false;
    }

    glAttachShader(program_, vertex_id);

    if (!compile_shader(GL_FRAGMENT_SHADER, fragment.c_str(), fragment_id)) {
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n";
        return false;
    }

    glAttachShader(program_, fragment_id);

    if (!geometry.empty()) {
        if (!compile_shader(GL_GEOMETRY_SHADER, geometry.c_str(), geometry_id)) {
            std::cerr << "ERROR::SHADER::GEOMETRY::COMPILATION_FAILED\n";
            return false;
        }

        glAttachShader(program_, geometry_id);
    }

    if (!link_program()) {
        std::cerr << "ERROR::PROGRAM::LINK_FAILED\n";
        return false;
    }

    glDeleteShader(vertex_id);
    glDeleteShader(fragment_id);

    if (!geometry.empty()) {
        glDeleteShader(geometry_id);
    }

    initialized_ = true;
    return true;
}

inline bool Shader::compile_shader(GLenum shader_type, char const * path, GLuint& handle)
{
    assert(path);

    std::cout << "loading shader: " << path << '\n';

    std::string code;
    std::ifstream infile;

    infile.exceptions(std::ifstream::badbit);

    try {
        infile.open(path);

        if (!infile.is_open()) {
            std::cerr << "ERROR::SHADER::FAILED_TO_OPEN_FILE: " << path << '\n';
            return false;
        }

        std::stringstream istream;
        istream << infile.rdbuf();
        infile.close();
        code = istream.str();
    }
    catch (std::ifstream::failure const& e) {
        std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << path << '\n';
        return false;
    }

    GLchar const * shader_code = code.c_str();
    handle = glCreateShader(shader_type);
    glShaderSource(handle, 1, &shader_code, nullptr);
    glCompileShader(handle);

    GLint success;
    glGetShaderiv(handle, GL_COMPILE_STATUS, &success);

    if (!success) {
        GLchar log[512];
        glGetShaderInfoLog(handle, sizeof(log), nullptr, log);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED: (" << path << ") - \n" << log << '\n';
        return false;
    }

    return true;
}

inline bool Shader::link_program()
{
    GLint success;
    glLinkProgram(program_);

    glGetProgramiv(program_, GL_LINK_STATUS, &success);

    if (!success) {
        GLchar log[512];
        glGetProgramInfoLog(program_, sizeof(log), NULL, log);
        std::cerr << log << '\n';
        return false;
    }

    return true;
}

inline GLint Shader::uniform_location(GLchar const * name) const
{
    assert(name);
    return glGetUniformLocation(program(), name);
}


inline void Shader::set_uniform_block_binding(char const * block, GLuint binding_point)
{
    GLuint index = glGetUniformBlockIndex(program(), block);
    glUniformBlockBinding(program(), index, binding_point);
}

inline void Shader::set_uniform(GLint location, glm::vec3 const& v)
{
    glUniform3fv(location, 1, &v[0]);
}

inline void Shader::set_uniform(GLint location, glm::vec2 const& v)
{
    glUniform2fv(location, 1, &v[0]);
}

inline void Shader::set_uniform(GLint location, glm::vec4 const& v)
{
    glUniform4fv(location, 1, &v[0]);
}

inline void Shader::set_uniform(GLint location, GLboolean b)
{
    glUniform1i(location, b);
}

inline void Shader::set_uniform(GLint location, glm::mat4 const& m)
{
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(m));
}

inline void Shader::set_uniform(GLint location, GLint i)
{
    glUniform1i(location, i);
}

inline void Shader::set_uniform(GLint location, GLuint i)
{
    glUniform1ui(location, i);
}

inline void Shader::set_uniform(GLint location, GLfloat f)
{
    glUniform1f(location, f);
}
