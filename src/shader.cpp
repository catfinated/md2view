#include "md2view/shader.hpp"

#include <array>
#include <fstream>
#include <stdexcept>
#include <utility>

Shader::Shader(std::filesystem::path const& vertex,
               std::filesystem::path const& fragment,
               std::optional<std::filesystem::path> const& geometry)
    : program_(0) {
    if (!init(vertex, fragment, geometry)) {
        throw std::runtime_error("shader initialization failed");
    }
}

Shader::~Shader() { cleanup(); }

Shader::Shader(Shader&& rhs) noexcept {
    program_ = std::exchange(rhs.program_, 0U);
}

Shader& Shader::operator=(Shader&& rhs) noexcept {
    if (this != &rhs) {
        cleanup();
        program_ = std::exchange(rhs.program_, 0U);
    }
    return *this;
}

void Shader::cleanup() {
    if (program_ > 0U) {
        glDeleteProgram(program_);
        program_ = 0U;
    }
}

bool Shader::init(std::filesystem::path const& vertex,
                  std::filesystem::path const& fragment,
                  std::optional<std::filesystem::path> const& geometry) {
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

    if (!compile_shader(GL_VERTEX_SHADER, vertex, vertex_id)) {
        spdlog::error("ERROR::SHADER::VERTEX::COMPILATION_FAILED");
        return false;
    }

    glAttachShader(program_, vertex_id);

    if (!compile_shader(GL_FRAGMENT_SHADER, fragment, fragment_id)) {
        spdlog::error("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED");
        return false;
    }

    glAttachShader(program_, fragment_id);

    if (geometry) {
        if (!compile_shader(GL_GEOMETRY_SHADER, geometry.value(),
                            geometry_id)) {
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

    if (geometry) {
        glDeleteShader(geometry_id);
    }

    return true;
}

bool Shader::compile_shader(GLenum shader_type,
                            std::filesystem::path const& path,
                            GLuint& handle) {
    spdlog::info("compiling '{}'", path.string());
    std::string code;

    try {
        std::ifstream infile;
        infile.exceptions(std::ifstream::badbit | std::ifstream::failbit);
        infile.open(path);
        std::stringstream istream;
        istream << infile.rdbuf();
        code = istream.str();
    } catch (std::ifstream::failure const& e) {
        spdlog::error("failed to read shader file '{}': {}", path.string(),
                      e.what());
        return false;
    }

    GLchar const* shader_code = code.c_str();
    handle = glCreateShader(shader_type);
    glShaderSource(handle, 1, &shader_code, nullptr);
    glCompileShader(handle);

    GLint success;
    glGetShaderiv(handle, GL_COMPILE_STATUS, &success);

    if (!success) {
        std::array<GLchar, 512> log{};
        glGetShaderInfoLog(handle, log.size(), nullptr, log.data());
        spdlog::error("ERROR::SHADER::COMPILATION_FAILED: ('{}') - \n{}",
                      path.string(), log.data());
        return false;
    }

    return true;
}

bool Shader::link_program() {
    GLint success;
    glLinkProgram(program_);

    glGetProgramiv(program_, GL_LINK_STATUS, &success);

    if (!success) {
        std::array<GLchar, 512> log{};
        glGetProgramInfoLog(program_, log.size(), nullptr, log.data());
        spdlog::error("{}", log.data());
        return false;
    }

    return true;
}

GLint Shader::uniform_location(gsl_lite::not_null<GLchar const*> name) const {
    return glGetUniformLocation(program(), name);
}

void Shader::set_uniform_block_binding(gsl_lite::not_null<char const*> block,
                                       GLuint binding_point) {
    GLuint index = glGetUniformBlockIndex(program(), block);
    glUniformBlockBinding(program(), index, binding_point);
}

void Shader::set_uniform(GLint location, glm::vec3 const& v) {
    glUniform3fv(location, 1, &v[0]);
}

void Shader::set_uniform(GLint location, glm::vec2 const& v) {
    glUniform2fv(location, 1, &v[0]);
}

void Shader::set_uniform(GLint location, glm::vec4 const& v) {
    glUniform4fv(location, 1, &v[0]);
}

void Shader::set_uniform(GLint location, GLboolean b) {
    glUniform1i(location, b);
}

void Shader::set_uniform(GLint location, glm::mat4 const& m) {
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(m));
}

void Shader::set_uniform(GLint location, GLint i) { glUniform1i(location, i); }

void Shader::set_uniform(GLint location, GLuint i) {
    glUniform1ui(location, i);
}

void Shader::set_uniform(GLint location, GLfloat f) {
    glUniform1f(location, f);
}
