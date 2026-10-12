#pragma once

#ifdef __APPLE__
#include <GL/glew.h>   // IWYU pragma: export
#include <OpenGL/gl.h> // IWYU pragma: export
#else
// clang-format off
#include <GL/glew.h> // IWYU pragma: export
#include <GL/gl.h>   // IWYU pragma: export
// clang-format on
#endif

#include <spdlog/spdlog.h>

#include <concepts>
#include <source_location>
#include <string_view>

inline const char* glErrorToString(GLenum error) {
    char const* str = "unknown error";

    switch (error) {
#define _TOK2CASE(token)                                                       \
    case token:                                                                \
        str = #token;                                                          \
        break
        _TOK2CASE(GL_NO_ERROR);
        _TOK2CASE(GL_INVALID_ENUM);
        _TOK2CASE(GL_INVALID_VALUE);
        _TOK2CASE(GL_INVALID_OPERATION);
        _TOK2CASE(GL_STACK_OVERFLOW);
        _TOK2CASE(GL_STACK_UNDERFLOW);
        _TOK2CASE(GL_OUT_OF_MEMORY);
        _TOK2CASE(GL_INVALID_FRAMEBUFFER_OPERATION);
#undef _TOK2CASE
    default:
        break;
    }

    return str;
}

inline GLenum
glCheckError(std::source_location loc = std::source_location::current()) {
    GLenum errorCode = glGetError();

    while (errorCode != GL_NO_ERROR) {
        spdlog::error("[{}:{}] {}", loc.file_name(), loc.line(),
                      glErrorToString(errorCode));
        errorCode = glGetError();
    }

    return errorCode;
}

void* glBufferOffset(std::integral auto offset) {
    return reinterpret_cast<void*>(offset);
}

inline std::string_view glStrView(GLubyte const* bytes) {
    return reinterpret_cast<char const*>(bytes);
}
