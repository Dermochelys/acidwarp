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

#set(BUILD_SHARED_LIBS OFF)

# Note we don't link PNG.  SDL3_image (As of November 2025) is compiled in a way 
# that doesn't allow PNG to be statically linked.  Therefore the .DLL file must 
# be distributed alongside the .exe file.

find_package(SDL3 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

add_subdirectory(acidwarp)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
add_definitions(-DGLEW_STATIC)

# Include the resource file to embed the icon and metadata
add_executable(acidwarp-windows acidwarp/acidwarp.c acidwarp.rc)

target_link_libraries(acidwarp-windows PRIVATE main SDL3::SDL3-static ZLIB::ZLIB SDL3_image::SDL3_image-static glew32 opengl32)
