find_package(Catch2 3 CONFIG REQUIRED)

add_executable(gen_fixtures gen_fixtures.cpp)

set(FIXTURE_DIR ${CMAKE_CURRENT_BINARY_DIR}/fixtures)
add_custom_command(
    OUTPUT
        ${FIXTURE_DIR}/minimal.pcx
        ${FIXTURE_DIR}/minimal.pak
    COMMAND gen_fixtures ${FIXTURE_DIR}
    DEPENDS gen_fixtures
    COMMENT "Generating test fixtures"
)
add_custom_target(test_fixtures
    DEPENDS ${FIXTURE_DIR}/minimal.pcx ${FIXTURE_DIR}/minimal.pak
)

configure_file(fixtures.hpp.in fixtures.hpp @ONLY)

add_executable(test_md2v
    test_camera.cpp
    test_md2.cpp
    test_pak.cpp
    test_pcx.cpp
    tmpdir.cpp
)
add_dependencies(test_md2v test_fixtures)

target_include_directories(test_md2v PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(test_md2v PRIVATE Catch2::Catch2WithMain libmd2v)

include(CTest)
include(Catch)
catch_discover_tests(test_md2v)
