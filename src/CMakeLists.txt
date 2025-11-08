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
find_package(tl-expected CONFIG REQUIRED)

add_library(libmd2v
  md2view.cpp
  gui.cpp
  md2.cpp
  pcx.cpp
  pak.cpp
  screen_quad.cpp
  resource_manager.cpp
  camera.cpp
  model_selector.cpp
  shader.cpp
  texture2D.cpp
  frame_buffer.cpp
  engine.cpp)

target_link_libraries(libmd2v PUBLIC
   Boost::system 
   Boost::program_options 
   glfw 
   GLEW::GLEW 
   glm::glm 
   OpenGL::GL 
   OpenGL::EGL
   imgui::imgui 
   spdlog::spdlog 
   gsl::gsl-lite-v1
)

target_include_directories(libmd2v PUBLIC ${OPENGL_INCLUDE_DIR} ${Stb_INCLUDE_DIR} ${TREEHH_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include)

target_compile_definitions(libmd2v
   PUBLIC GLM_ENABLE_EXPERIMENTAL GLEW_NO_GLU FMT_USE_USER_DEFINED_LITERALS=1 IMGUI_DISABLE_OBSOLETE_FUNCTIONS gsl_CONFIG_CONTRACT_VIOLATION_THROWS
)

add_executable(glmd2v main.cpp)

target_link_libraries(glmd2v PRIVATE libmd2v)

add_executable(vkmd2v vk.cpp vkengine.cpp vkmain.cpp)

target_link_libraries(vkmd2v PRIVATE libmd2v Vulkan::Vulkan glfw)

target_compile_definitions(vkmd2v PUBLIC GLFW_INCLUDE_VULKAN)
