cmake_minimum_required(VERSION 3.28.3)
project(acidwarp-windows)

# Linux-specific build configuration
if(NOT WIN32)
    message(FATAL_ERROR "This CMakeLists.txt is designed for Windows builds only")
endif()

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "This CMakeLists.txt is designed for GCC builds only")
endif()

include(FindPkgConfig)
pkg_search_module(SDL REQUIRED sdl3)
include_directories(${SDL_INCLUDE_DIRS})

# Find OpenGL for desktop Linux
find_package(OpenGL REQUIRED)
if(NOT OPENGL_FOUND)
    message(FATAL_ERROR "OpenGL 4.1+ is required for Windows builds")
endif()

add_subdirectory(acidwarp)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
add_definitions(-DGLEW_STATIC)

# Create an executable that links to the main library from the submodule
# Include the resource file to embed the icon
add_executable(acidwarp-windows acidwarp/acidwarp.c acidwarp.rc)

# Link against the library and OpenGL for desktop
target_link_libraries(acidwarp-windows PRIVATE main ${SDL_STATIC_LDFLAGS} glew32 opengl32)
