#pragma once

#include "gl.hpp"

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
