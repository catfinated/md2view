#include "shader.hpp"

#include <fstream>
#include <stdexcept>

 Shader::Shader(std::string const& vertex,
                std::string const& fragment,
                std::string const& geometry)
    : program_(0)
{
    if (!init(vertex, fragment, geometry)) {
        throw std::runtime_error("shader initialization failed");
    }
}

 Shader::~Shader()
{
    cleanup();
}

 Shader::Shader(Shader&& rhs)
{
    program_ = rhs.program_;
    initialized_ = rhs.initialized_;

    rhs.program_ = 0;
    rhs.initialized_ = false;
}

 Shader& Shader::operator=(Shader&& rhs)
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

 void Shader::cleanup()
{
    if (initialized_) {
        glDeleteProgram(program_);
        initialized_ = false;
    }
}

 bool Shader::init(std::string const& vertex,
                   std::string const& fragment,
                   std::string const& geometry)
{
    if (vertex.empty()) {
        spdlog::error("shader vertex path cannot be empty");
        return false;
    }

    if (fragment.empty()) {
        spdlog::error("shader fragment path cannot be empty");
        return false;
    }

    program_ = glCreateProgram();

    GLuint vertex_id, fragment_id, geometry_id;

    if (!compile_shader(GL_VERTEX_SHADER, vertex.c_str(), vertex_id)) {
        spdlog::error("ERROR::SHADER::VERTEX::COMPILATION_FAILED");
        return false;
    }

    glAttachShader(program_, vertex_id);

    if (!compile_shader(GL_FRAGMENT_SHADER, fragment.c_str(), fragment_id)) {
        spdlog::error("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED");
        return false;
    }

    glAttachShader(program_, fragment_id);

    if (!geometry.empty()) {
        if (!compile_shader(GL_GEOMETRY_SHADER, geometry.c_str(), geometry_id)) {
            spdlog::error("ERROR::SHADER::GEOMETRY::COMPILATION_FAILED");
            return false;
        }

        glAttachShader(program_, geometry_id);
    }

    if (!link_program()) {
        spdlog::error("ERROR::PROGRAM::LINK_FAILED");
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

 bool Shader::compile_shader(GLenum shader_type, char const * path, GLuint& handle)
{
    assert(path);

    spdlog::info("loading shader: {}", path);

    std::string code;
    std::ifstream infile;

    infile.exceptions(std::ifstream::badbit);

    try {
        infile.open(path);

        if (!infile.is_open()) {
            spdlog::error("ERROR::SHADER::FAILED_TO_OPEN_FILE: {}", path);
            return false;
        }

        std::stringstream istream;
        istream << infile.rdbuf();
        infile.close();
        code = istream.str();
    }
    catch (std::ifstream::failure const& e) {
        spdlog::error("ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: {}", path);
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
        spdlog::error("ERROR::SHADER::COMPILATION_FAILED: ('{}') - \n{}", path, log);
        return false;
    }

    return true;
}

 bool Shader::link_program()
{
    GLint success;
    glLinkProgram(program_);

    glGetProgramiv(program_, GL_LINK_STATUS, &success);

    if (!success) {
        GLchar log[512];
        glGetProgramInfoLog(program_, sizeof(log), NULL, log);
        spdlog::error("{}", log);
        return false;
    }

    return true;
}

 GLint Shader::uniform_location(GLchar const * name) const
{
    assert(name);
    return glGetUniformLocation(program(), name);
}


 void Shader::set_uniform_block_binding(char const * block, GLuint binding_point)
{
    GLuint index = glGetUniformBlockIndex(program(), block);
    glUniformBlockBinding(program(), index, binding_point);
}

 void Shader::set_uniform(GLint location, glm::vec3 const& v)
{
    glUniform3fv(location, 1, &v[0]);
}

 void Shader::set_uniform(GLint location, glm::vec2 const& v)
{
    glUniform2fv(location, 1, &v[0]);
}

 void Shader::set_uniform(GLint location, glm::vec4 const& v)
{
    glUniform4fv(location, 1, &v[0]);
}

 void Shader::set_uniform(GLint location, GLboolean b)
{
    glUniform1i(location, b);
}

 void Shader::set_uniform(GLint location, glm::mat4 const& m)
{
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(m));
}

 void Shader::set_uniform(GLint location, GLint i)
{
    glUniform1i(location, i);
}

 void Shader::set_uniform(GLint location, GLuint i)
{
    glUniform1ui(location, i);
}

 void Shader::set_uniform(GLint location, GLfloat f)
{
    glUniform1f(location, f);
}
