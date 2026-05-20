cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project("md2view" VERSION 1.0 LANGUAGES CXX)

if(CMAKE_EXPORT_COMPILE_COMMANDS)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/compile_commands.json
    )
endif()

add_subdirectory(src)
enable_testing()
add_subdirectory(tests)
