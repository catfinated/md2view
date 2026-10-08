find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glew REQUIRED)
find_package(OpenGL REQUIRED COMPONENTS GLX EGL OpenGL)
find_package(Boost COMPONENTS system program_options REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(gsl-lite CONFIG REQUIRED)
find_path(TREEHH_INCLUDE_DIRS "treehh/tree.hh")
find_package(Vulkan REQUIRED)

add_library(md2v
  md2.cpp
  pcx.cpp
  pak.cpp
  image.cpp
  camera.cpp)

target_link_libraries(md2v PUBLIC
   Boost::system
   Boost::program_options
   glm::glm
   spdlog::spdlog
   gsl::gsl-lite-v1)

target_include_directories(md2v PUBLIC 
  ${CMAKE_SOURCE_DIR}/include
  ${Stb_INCLUDE_DIR})

target_compile_definitions(md2v
   PUBLIC GLM_ENABLE_EXPERIMENTAL FMT_USE_USER_DEFINED_LITERALS=1 gsl_CONFIG_CONTRACT_VIOLATION_THROWS)

# GLFW-based application layer shared by the renderer backends: window
# ownership, input dispatch, and the Engine base class.
add_library(md2vw
  window.cpp
  engine.cpp)

target_link_libraries(md2vw PUBLIC
   md2v
   glfw)

add_library(md2vui
  ui/ui.cpp
  ui/model_selector.cpp)

target_link_libraries(md2vui PUBLIC
   md2v
   imgui::imgui)

target_include_directories(md2vui PUBLIC ${TREEHH_INCLUDE_DIRS})

target_compile_definitions(md2vui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)

add_library(md2vgl
  gl/md2view.cpp
  gl/gui.cpp
  gl/mesh.cpp
  gl/screen_quad.cpp
  gl/resource_manager.cpp
  gl/shader.cpp
  gl/texture2d.cpp
  gl/frame_buffer.cpp
  gl/renderer.cpp)

target_link_libraries(md2vgl PUBLIC
   md2vui
   md2vw
   GLEW::GLEW
   OpenGL::GL
   OpenGL::EGL
   imgui::imgui)

target_include_directories(md2vgl PUBLIC ${OPENGL_INCLUDE_DIR})

target_compile_definitions(md2vgl
   PUBLIC GLEW_NO_GLU IMGUI_DISABLE_OBSOLETE_FUNCTIONS)

add_library(md2vvk
  vk/vk.cpp
  vk/engine.cpp
  vk/renderer.cpp)

target_link_libraries(md2vvk PUBLIC
   md2v
   md2vw
   Vulkan::Vulkan)

target_compile_definitions(md2vvk PUBLIC GLFW_INCLUDE_VULKAN)

add_executable(glmd2v gl/main.cpp)
target_link_libraries(glmd2v PRIVATE md2vgl)

add_executable(vkmd2v vk/main.cpp)
target_link_libraries(vkmd2v PRIVATE md2vvk)
