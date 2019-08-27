#pragma once

#ifdef __APPLE__
#include <GL/glew.h>
#include <OpenGL/gl.h>
#else
#include <GL/glew.h>
#include <GL/gl.h>
#endif

#include <cassert>
#include <cstdlib>
#include <iostream>

#define BLUE_PANIC(expr)                        \
    do { \
        std::cerr << "[" << __FILE__ << ":" << __LINE__ << "] panic! - " << expr << '\n'; \
        abort(); \
    } while (0)

#define BLUE_EXPECT(cond) \
    do { \
        if (!(cond)) { \
            BLUE_PANIC("expect failed: " << #cond);   \
        } \
    } while (0)

#define BLUE_SYSCALL(syscall) \
    do { \
        int result = syscall; \
        if (-1 == result) { \
            char buf[256]; \
            strerror_r(result, buf, sizeof(buf)); \
            BLUE_PANIC(#syscall << " (errno: " << buf << ")"); \
        } \
    } while (0)

#define BLUE_GL_BUFFER_OFFSET(offset) (reinterpret_cast<void *>(offset))

namespace blue {

inline const char * glErrorToString(GLenum error)
{
    char const * str = "unknown error";

    switch (error) {
#define _BLUE_CASE(token) case token: str = #token; break
     _BLUE_CASE(GL_NO_ERROR);
     _BLUE_CASE(GL_INVALID_ENUM);
     _BLUE_CASE(GL_INVALID_VALUE);
     _BLUE_CASE(GL_INVALID_OPERATION);
     _BLUE_CASE(GL_STACK_OVERFLOW);
     _BLUE_CASE(GL_STACK_UNDERFLOW);
     _BLUE_CASE(GL_OUT_OF_MEMORY);
     _BLUE_CASE(GL_INVALID_FRAMEBUFFER_OPERATION);
#undef _BLUE_CASE
    default:
        break;
    }

    return str;
}

inline GLenum glCheckError(char const * file, int line)
{
    assert(file);

    GLenum error_code = glGetError();

    while (error_code != GL_NO_ERROR) {
        std::cerr << "[" << file << ":" << line << "] " << glErrorToString(error_code) << '\n';
        error_code = glGetError();
    }

    return error_code;
}

} // namespace blue

#define glCheckError() blue::glCheckError(__FILE__, __LINE__)
