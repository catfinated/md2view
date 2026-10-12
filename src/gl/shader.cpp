#include "md2view/gl/shader.hpp"
#include "md2view/gl/gl.hpp"

#include <fmt/std.h>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <gsl-lite/gsl-lite.hpp>
#include <spdlog/spdlog.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace GL {

Shader::Shader(std::filesystem::path const& vertex,
               std::filesystem::path const& fragment,
               std::optional<std::filesystem::path> const& geometry) {
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

    GLuint vertexId{};
    GLuint fragmentId{};
    GLuint geometryId{};

    if (!compileShader(GL_VERTEX_SHADER, vertex, vertexId)) {
        spdlog::error("ERROR::SHADER::VERTEX::COMPILATION_FAILED");
        return false;
    }

    glAttachShader(program_, vertexId);

    if (!compileShader(GL_FRAGMENT_SHADER, fragment, fragmentId)) {
        spdlog::error("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED");
        return false;
    }

    glAttachShader(program_, fragmentId);

    if (geometry) {
        if (!compileShader(GL_GEOMETRY_SHADER, geometry.value(), geometryId)) {
            spdlog::error("ERROR::SHADER::GEOMETRY::COMPILATION_FAILED");
            return false;
        }

        glAttachShader(program_, geometryId);
    }

    if (!linkProgram(program_)) {
        spdlog::error("ERROR::PROGRAM::LINK_FAILED");
        return false;
    }

    glDeleteShader(vertexId);
    glDeleteShader(fragmentId);

    if (geometry) {
        glDeleteShader(geometryId);
    }

    return true;
}

bool Shader::compileShader(GLenum shaderType,
                           std::filesystem::path const& path,
                           GLuint& handle) {
    spdlog::info("compiling '{}'", path);
    std::string code;

    try {
        std::ifstream infile;
        infile.exceptions(std::ifstream::badbit | std::ifstream::failbit);
        infile.open(path);
        std::stringstream istream;
        istream << infile.rdbuf();
        code = istream.str();
    } catch (std::ifstream::failure const& e) {
        spdlog::error("failed to read shader file '{}': {}", path, e.what());
        return false;
    }

    GLchar const* shaderCode = code.c_str();
    handle = glCreateShader(shaderType);
    glShaderSource(handle, 1, &shaderCode, nullptr);
    glCompileShader(handle);

    GLint success{};
    glGetShaderiv(handle, GL_COMPILE_STATUS, &success);

    if (success == GL_FALSE) {
        std::array<GLchar, 512> log{};
        glGetShaderInfoLog(handle, log.size(), nullptr, log.data());
        spdlog::error("ERROR::SHADER::COMPILATION_FAILED: ('{}') - \n{}", path,
                      log.data());
        return false;
    }

    return true;
}

bool Shader::linkProgram(GLuint program) {
    GLint success{};
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (success == GL_FALSE) {
        std::array<GLchar, 512> log{};
        glGetProgramInfoLog(program, log.size(), nullptr, log.data());
        spdlog::error("{}", log.data());
        return false;
    }

    return true;
}

GLint Shader::uniformLocation(gsl_lite::not_null<GLchar const*> name) const {
    return glGetUniformLocation(program(), name);
}

void Shader::setUniformBlockBinding(gsl_lite::not_null<char const*> block,
                                    GLuint bindingPoint) const {
    GLuint index = glGetUniformBlockIndex(program(), block);
    glUniformBlockBinding(program(), index, bindingPoint);
}

void Shader::setUniform(GLint location, glm::vec3 const& v) {
    glUniform3fv(location, 1, &v[0]);
}

void Shader::setUniform(GLint location, glm::vec2 const& v) {
    glUniform2fv(location, 1, &v[0]);
}

void Shader::setUniform(GLint location, glm::vec4 const& v) {
    glUniform4fv(location, 1, &v[0]);
}

void Shader::setUniform(GLint location, GLboolean b) {
    glUniform1i(location, b);
}

void Shader::setUniform(GLint location, glm::mat4 const& m) {
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(m));
}

void Shader::setUniform(GLint location, GLint i) { glUniform1i(location, i); }

void Shader::setUniform(GLint location, GLuint i) { glUniform1ui(location, i); }

void Shader::setUniform(GLint location, GLfloat f) { glUniform1f(location, f); }

} // namespace GL
