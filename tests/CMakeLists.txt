find_package(Catch2 3 CONFIG REQUIRED)

add_executable(test_md2v
    test_camera.cpp
    test_md2.cpp
    test_pak.cpp
    test_pcx.cpp
    tmpdir.cpp
)

target_include_directories(test_md2v PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_md2v PRIVATE Catch2::Catch2WithMain libmd2v)

include(CTest)
include(Catch)
catch_discover_tests(test_md2v)
