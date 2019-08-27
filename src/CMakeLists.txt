cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project("md2view" VERSION 1.0 LANGUAGES CXX)

find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(glew REQUIRED CONFIG)
find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)

SET(TOOLCHAIN_DIR /Users/nick/projects/toolchain/tc1)
#find_library(SOIL_LIBRARY soil NO_DEFAULT_PATH PATHS ${TOOLCHAIN_DIR}/SOIL/lib)
find_library(SOIL_LIBRARY soil)
SET(SOIL_INCLUDE ${TOOLCHAIN_DIR}/SOIL/include)

find_package(Boost COMPONENTS system filesystem program_options REQUIRED)
add_library(boost INTERFACE IMPORTED)
set_property(TARGET boost PROPERTY
    INTERFACE_INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIR})

add_library(boost-system SHARED IMPORTED)
set_property(TARGET boost-system PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIRS})
set_property(TARGET boost-system PROPERTY IMPORTED_LOCATION ${Boost_SYSTEM_LIBRARY})

add_library(boost-filesystem SHARED IMPORTED)
set_property(TARGET boost-filesystem PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIRS})
set_property(TARGET boost-filesystem PROPERTY IMPORTED_LOCATION ${Boost_FILESYSTEM_LIBRARY})

add_library(boost-program-options SHARED IMPORTED)
set_property(TARGET boost-program-options PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIRS})
set_property(TARGET boost-program-options PROPERTY IMPORTED_LOCATION ${Boost_PROGRAM_OPTIONS_LIBRARY})

IF(APPLE)
   find_library(COCOA_LIBRARY Cocoa)
   find_library(IOKIT_LIBRARY IOKit)
   find_library(COREVIDEO_LIBRARY CoreVideo)
   SET(PLATFORM_LIBS ${COCOA_LIBRARY} ${IOKIT_LIBRARY} ${COREVIDEO_LIBRARY})
ENDIF(APPLE)

get_cmake_property(_variableNames VARIABLES)
list (SORT _variableNames)
foreach (_variableName ${_variableNames})
    message(STATUS "${_variableName}=${${_variableName}}")
endforeach()

include_directories(".")
include_directories("imgui")

add_executable(md2view
  md2view.cpp
  gui.cpp
  md2.cpp
  imgui/imgui.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_demo.cpp)

set_target_properties(md2view PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_link_libraries(md2view
   PUBLIC ${BOOST_LIBRARIES} boost-system boost-filesystem boost-program-options glfw GLEW::glew_s glm ${SOIL_LIBRARY} ${PLATFORM_LIBS}
)

target_include_directories(md2view
   PUBLIC include ${Boost_INCLUDE_DIR} ${SOIL_INCLUDE}
)

target_compile_options(md2view
   PUBLIC -Wall -pedantic -Wno-unused-function -Wno-unused-local-typedefs -Wno-missing-braces -Wmissing-field-initializers -DGLM_ENABLE_EXPERIMENTAL -DGLEW_NO_GLU -DBOOST_ERROR_CODE_HEADER_ONLY -g
)
