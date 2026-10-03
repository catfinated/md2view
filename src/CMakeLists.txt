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
  camera.cpp
  engine.cpp)

target_link_libraries(libmd2 PUBLIC
   Boost::system
   Boost::program_options
   glm::glm
   spdlog::spdlog
   gsl::gsl-lite-v1)

target_include_directories(libmd2 PUBLIC ${CMAKE_SOURCE_DIR}/include)

target_compile_definitions(libmd2
   PUBLIC GLM_ENABLE_EXPERIMENTAL FMT_USE_USER_DEFINED_LITERALS=1 gsl_CONFIG_CONTRACT_VIOLATION_THROWS)

# Backend-agnostic ImGui layer: panels that depend only on the ImGui widget API
# and the core domain types, usable from any renderer backend.
add_library(libmd2ui
  ui/ui.cpp
  ui/model_selector.cpp)

target_link_libraries(libmd2ui PUBLIC
   libmd2
   imgui::imgui)

target_include_directories(libmd2ui PUBLIC ${TREEHH_INCLUDE_DIRS})

target_compile_definitions(libmd2ui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)

# OpenGL rendering layer: shaders, textures, meshes, GUI, resource manager, app.
add_library(libmd2gl
  gl/md2view.cpp
  gl/gui.cpp
  gl/mesh.cpp
  gl/screen_quad.cpp
  gl/resource_manager.cpp
  gl/shader.cpp
  gl/texture2d.cpp
  gl/frame_buffer.cpp)

target_link_libraries(libmd2gl PUBLIC
   libmd2ui
   glfw
   GLEW::GLEW
   OpenGL::GL
   OpenGL::EGL
   imgui::imgui)

target_include_directories(libmd2gl PUBLIC
   ${OPENGL_INCLUDE_DIR}
   ${Stb_INCLUDE_DIR})

target_compile_definitions(libmd2gl
   PUBLIC GLEW_NO_GLU IMGUI_DISABLE_OBSOLETE_FUNCTIONS)

# Vulkan rendering layer: instance/device/swapchain setup, sync, engine loop.
add_library(libmd2vk
  vk/vk.cpp
  vk/engine.cpp)

target_link_libraries(libmd2vk PUBLIC
   libmd2
   Vulkan::Vulkan
   glfw)

target_compile_definitions(libmd2vk PUBLIC GLFW_INCLUDE_VULKAN)

add_executable(glmd2v gl/main.cpp)
target_link_libraries(glmd2v PRIVATE libmd2gl)

add_executable(vkmd2v vk/main.cpp)
target_link_libraries(vkmd2v PRIVATE libmd2vk)
