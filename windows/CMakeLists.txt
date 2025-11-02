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
pkg_search_module(SDL_IMAGE sdl3_image SDL3_image)
if(NOT SDL_IMAGE_FOUND)
    # Fallback: try to find SDL3_image manually
    find_library(SDL3_IMAGE_LIBRARY NAMES SDL3_image libSDL3_image)
    find_path(SDL3_IMAGE_INCLUDE_DIR NAMES SDL3/SDL_image.h)
    if(SDL3_IMAGE_LIBRARY AND SDL3_IMAGE_INCLUDE_DIR)
        set(SDL_IMAGE_FOUND TRUE)
        set(SDL_IMAGE_LIBRARIES ${SDL3_IMAGE_LIBRARY})
        set(SDL_IMAGE_INCLUDE_DIRS ${SDL3_IMAGE_INCLUDE_DIR})
        set(SDL_IMAGE_STATIC_LDFLAGS ${SDL3_IMAGE_LIBRARY})
        message(STATUS "Found SDL3_image manually: ${SDL3_IMAGE_LIBRARY}")
    else()
        message(FATAL_ERROR "SDL3_image not found via pkg-config or manual search")
    endif()
endif()
include_directories(${SDL_INCLUDE_DIRS} ${SDL_IMAGE_INCLUDE_DIRS})

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
target_link_libraries(acidwarp-windows PRIVATE main ${SDL_STATIC_LDFLAGS} ${SDL_IMAGE_STATIC_LDFLAGS} glew32 opengl32)
