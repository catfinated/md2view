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

find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    set(DOXYGEN_PROJECT_NAME        "md2view")
    set(DOXYGEN_PROJECT_BRIEF       "Quake II MD2 model viewer")
    set(DOXYGEN_OUTPUT_DIRECTORY    ${CMAKE_BINARY_DIR}/docs)
    set(DOXYGEN_GENERATE_HTML       YES)
    set(DOXYGEN_GENERATE_LATEX      NO)
    set(DOXYGEN_EXTRACT_ALL         YES)
    set(DOXYGEN_MARKDOWN_SUPPORT    YES)
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE ${CMAKE_SOURCE_DIR}/README.md)

    # Doxygen Awesome theme
    set(DOXYGEN_HTML_EXTRA_STYLESHEET
        ${CMAKE_SOURCE_DIR}/docs/doxygen-awesome.css
        ${CMAKE_SOURCE_DIR}/docs/doxygen-awesome-sidebar-only.css)
    set(DOXYGEN_HTML_COLORSTYLE     TOGGLE)  # built-in dark/light toggle
    set(DOXYGEN_GENERATE_TREEVIEW   YES)     # required for sidebar layout
    set(DOXYGEN_DISABLE_INDEX       NO)
    set(DOXYGEN_FULL_SIDEBAR        NO)

    set(DOXYGEN_EXCLUDE_PATTERNS    "${CMAKE_SOURCE_DIR}/build/*")
    set(DOXYGEN_EXCLUDE_SYMBOLS     "fmt::*")

    doxygen_add_docs(docs
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/README.md
        ${CMAKE_SOURCE_DIR}/docs/DESIGN.md
        COMMENT "Generating HTML documentation")
endif()
