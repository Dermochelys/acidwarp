cmake_minimum_required(VERSION 3.28.3)

project(acidwarp-linux)

# Linux-specific build configuration
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "This CMakeLists.txt is designed for Linux builds only")
endif()

# Find SDL3 - required dependency
find_package(SDL3 CONFIG)
if(NOT TARGET SDL3::SDL3)
    find_library(SDL3_LIBRARY NAMES "SDL3")
    find_path(SDL3_INCLUDE_DIR NAMES "SDL3/SDL.h")
    if(SDL3_LIBRARY AND SDL3_INCLUDE_DIR)
        add_library(SDL3::SDL3 UNKNOWN IMPORTED)
        set_property(TARGET SDL3::SDL3 PROPERTY IMPORTED_LOCATION "${SDL3_LIBRARY}")
        set_property(TARGET SDL3::SDL3 PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${SDL3_INCLUDE_DIR}")
    endif()
endif()

if(NOT TARGET SDL3::SDL3)
    message(FATAL_ERROR "SDL3 is required for this build. Please install SDL3 development libraries.")
endif()

# Find SDL3_image - required dependency
find_package(SDL3_image CONFIG)
if(NOT TARGET SDL3_image::SDL3_image)
    find_library(SDL3_IMAGE_LIBRARY NAMES "SDL3_image")
    find_path(SDL3_IMAGE_INCLUDE_DIR NAMES "SDL3/SDL_image.h")
    if(SDL3_IMAGE_LIBRARY AND SDL3_IMAGE_INCLUDE_DIR)
        add_library(SDL3_image::SDL3_image UNKNOWN IMPORTED)
        set_property(TARGET SDL3_image::SDL3_image PROPERTY IMPORTED_LOCATION "${SDL3_IMAGE_LIBRARY}")
        set_property(TARGET SDL3_image::SDL3_image PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${SDL3_IMAGE_INCLUDE_DIR}")
    endif()
endif()

if(NOT TARGET SDL3_image::SDL3_image)
    message(FATAL_ERROR "SDL3_image is required for this build. Please install SDL3_image development libraries.")
endif()

# Find OpenGL for desktop Linux
find_package(OpenGL REQUIRED)
if(NOT OPENGL_FOUND)
    message(FATAL_ERROR "OpenGL 4.1+ is required for Linux desktop builds")
endif()

# Add the acidwarp source directory
add_subdirectory(acidwarp)

# Create an executable that links to the main library
add_executable(acidwarp-linux acidwarp/acidwarp.c)

# Link against the library and OpenGL for desktop
target_link_libraries(acidwarp-linux PRIVATE main ${OPENGL_LIBRARIES} SDL3::SDL3 SDL3_image::SDL3_image m)

# Security: Disable executable stack
target_link_options(acidwarp-linux PRIVATE "-Wl,-z,noexecstack")

# Install targets
install(TARGETS acidwarp-linux DESTINATION bin)
install(FILES acidwarp.desktop DESTINATION share/applications)
