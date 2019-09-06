#pragma once

#include <cassert>
#include <cstdlib>
#include <iostream>

#define MD2V_PANIC(expr)                        \
    do { \
        std::cerr << "[" << __FILE__ << ":" << __LINE__ << "] panic! - " << expr << '\n'; \
        abort(); \
    } while (0)

#define MD2V_EXPECT(cond) \
    do { \
        if (!(cond)) { \
            MD2V_PANIC("expect failed: " << #cond); \
        } \
    } while (0)
