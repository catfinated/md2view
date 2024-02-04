find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(glew REQUIRED CONFIG)
find_package(OpenGL REQUIRED)
find_package(Boost COMPONENTS system filesystem program_options REQUIRED)

#IF(APPLE)
#   find_library(COCOA_LIBRARY Cocoa)
#   find_library(IOKIT_LIBRARY IOKit)
#   find_library(COREVIDEO_LIBRARY CoreVideo)
#   SET(PLATFORM_LIBS ${COCOA_LIBRARY} ${IOKIT_LIBRARY} ${COREVIDEO_LIBRARY})
#ENDIF(APPLE)

add_executable(md2view
  md2view.cpp
  gui.cpp
  md2.cpp
  pcx.cpp
  pak.cpp
  screen_quad.cpp
  imgui/imgui.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_demo.cpp)

set_target_properties(md2view PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_link_libraries(md2view
   PUBLIC Boost::system Boost::filesystem Boost::program_options glfw GLEW::GLEW glm::glm OpenGL::GL
)

target_include_directories(md2view PRIVATE ${OPENGL_INCLUDE_DIR})

target_compile_options(md2view
   PUBLIC -DGLM_ENABLE_EXPERIMENTAL -DGLEW_NO_GLU
)
