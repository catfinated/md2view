find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(glew REQUIRED CONFIG)
find_package(OpenGL REQUIRED)
find_package(Boost COMPONENTS system program_options REQUIRED)
find_package(imgui REQUIRED)
find_package(spdlog REQUIRED)
find_package(imgui REQUIRED)
find_package(Stb REQUIRED)
find_package(gsl-lite REQUIRED)
find_package(range-v3 CONFIG REQUIRED)
find_path(TREEHH_INCLUDE_DIRS "treehh/tree.hh")
#find_package(Vulkan REQUIRED)

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
   imgui::imgui 
   spdlog::spdlog 
   gsl::gsl-lite-v1
   range-v3::meta 
   range-v3::concepts 
   range-v3::range-v3
)

target_include_directories(libmd2v PUBLIC ${OPENGL_INCLUDE_DIR} ${Stb_INCLUDE_DIR} ${TREEHH_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include)

target_compile_options(libmd2v
   PUBLIC -DGLM_ENABLE_EXPERIMENTAL -DGLEW_NO_GLU -DFMT_USE_USER_DEFINED_LITERALS=1 -DIMGUI_DISABLE_OBSOLETE_FUNCTIONS
)

add_executable(glmd2v main.cpp)

target_link_libraries(glmd2v PRIVATE libmd2v)

#add_executable(vkmd2v vk.cpp vkengine.cpp vkmain.cpp)

#target_link_libraries(vkmd2v PRIVATE libmd2v Vulkan::Vulkan)

#target_compile_options(vkmd2v PUBLIC -DGLFW_INCLUDE_VULKAN)
