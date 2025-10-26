cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project("md2view" VERSION 1.0 LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 23)
set(CXX_STANDARD_REQUIRED YES)
set(CXX_EXTENSIONS NO)
add_compile_options(-Wall -Wextra -Wpedantic -Werror)

add_subdirectory(src)
