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

# Core library: MD2 parsing, camera math, PCX/PAK I/O — no OpenGL dependency.
add_library(libmd2
  md2.cpp
  pcx.cpp
  pak.cpp
  camera.cpp)

target_link_libraries(libmd2 PUBLIC
   Boost::system
   glm::glm
   spdlog::spdlog
   gsl::gsl-lite-v1)

target_include_directories(libmd2 PUBLIC ${CMAKE_SOURCE_DIR}/include)

target_compile_definitions(libmd2
   PUBLIC GLM_ENABLE_EXPERIMENTAL FMT_USE_USER_DEFINED_LITERALS=1 gsl_CONFIG_CONTRACT_VIOLATION_THROWS)

# OpenGL rendering layer: shaders, textures, meshes, GUI, resource manager, app.
add_library(libmd2gl
  md2view.cpp
  gl_gui.cpp
  gl_mesh.cpp
  gl_screen_quad.cpp
  resource_manager.cpp
  model_selector.cpp
  ui.cpp
  gl_shader.cpp
  gl_texture2d.cpp
  gl_frame_buffer.cpp
  engine.cpp)

target_link_libraries(libmd2gl PUBLIC
   libmd2
   glfw
   GLEW::GLEW
   OpenGL::GL
   OpenGL::EGL
   imgui::imgui
   Boost::program_options)

target_include_directories(libmd2gl PUBLIC
   ${OPENGL_INCLUDE_DIR}
   ${Stb_INCLUDE_DIR}
   ${TREEHH_INCLUDE_DIRS})

target_compile_definitions(libmd2gl
   PUBLIC GLEW_NO_GLU IMGUI_DISABLE_OBSOLETE_FUNCTIONS)

add_executable(glmd2v main.cpp)
target_link_libraries(glmd2v PRIVATE libmd2gl)

add_executable(vkmd2v vk.cpp vkengine.cpp vkmain.cpp)
target_link_libraries(vkmd2v PRIVATE libmd2 Vulkan::Vulkan glfw)
target_compile_definitions(vkmd2v PUBLIC GLFW_INCLUDE_VULKAN)
