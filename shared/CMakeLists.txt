cmake_minimum_required(VERSION 3.28.3)

project(acidwarp)

if(APPLE)
    message(FATAL_ERROR "CMake is not supported for iOS / iPadOS / macOS")
elseif(WIN32)
    include(FindPkgConfig)
    pkg_search_module(SDL REQUIRED sdl3)
    pkg_search_module(SDL_IMAGE sdl3_image SDL3_image)
    include_directories(${SDL_INCLUDE_DIRS} ${SDL_IMAGE_INCLUDE_DIRS})
elseif(ANDROID OR UNIX)
    if(NOT TARGET SDL3::SDL3)
        find_package(SDL3 CONFIG)
    endif()

    if(NOT TARGET SDL3::SDL3)
        find_library(SDL3_LIBRARY NAMES "SDL3")
        find_path(SDL3_INCLUDE_DIR NAMES "SDL3/SDL.h")
        add_library(SDL3::SDL3 UNKNOWN IMPORTED)
        set_property(TARGET SDL3::SDL3 PROPERTY IMPORTED_LOCATION "${SDL3_LIBRARY}")
        set_property(TARGET SDL3::SDL3 PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${SDL3_INCLUDE_DIR}")
    endif()

    if(NOT TARGET SDL3::SDL3)
        message(FATAL_ERROR "Cannot find SDL3.")
    endif()

    # Find SDL3_image
    if(NOT TARGET SDL3_image::SDL3_image)
        find_package(SDL3_image CONFIG)
    endif()

    if(NOT TARGET SDL3_image::SDL3_image)
        find_library(SDL3_IMAGE_LIBRARY NAMES "SDL3_image")
        find_path(SDL3_IMAGE_INCLUDE_DIR NAMES "SDL3/SDL_image.h")
        add_library(SDL3_image::SDL3_image UNKNOWN IMPORTED)
        set_property(TARGET SDL3_image::SDL3_image PROPERTY IMPORTED_LOCATION "${SDL3_IMAGE_LIBRARY}")
        set_property(TARGET SDL3_image::SDL3_image PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${SDL3_IMAGE_INCLUDE_DIR}")
    endif()

    if(NOT TARGET SDL3_image::SDL3_image)
        message(FATAL_ERROR "Cannot find SDL3_image.")
    endif()
else()
    message(FATAL_ERROR "CMake is not supported for this platform.")
endif()

# Android: To add logging, use this:
#find_library(log-lib log)

set(SOURCES acidwarp.c bit_map.c display.c draw.c gen_img.c palinit.c rolnfade.c remote_overlay.c)

# Embed remote.png as a binary resource for Linux
if(UNIX AND NOT ANDROID)
    set(REMOTE_PNG_PATH "${CMAKE_CURRENT_SOURCE_DIR}/remote.png")
    set(REMOTE_PNG_OBJ "${CMAKE_CURRENT_BINARY_DIR}/remote_png.o")

    add_custom_command(
        OUTPUT ${REMOTE_PNG_OBJ}
        COMMAND ${CMAKE_COMMAND} -E copy "${REMOTE_PNG_PATH}" remote.png
        COMMAND ${CMAKE_OBJCOPY} --input binary --output elf64-x86-64
                --binary-architecture i386:x86-64
                remote.png remote_png.o
        COMMAND ${CMAKE_OBJCOPY}
                --redefine-sym _binary_remote_png_start=_binary_remote_png_start
                --redefine-sym _binary_remote_png_end=_binary_remote_png_end
                --redefine-sym _binary_remote_png_size=_binary_remote_png_size
                remote_png.o
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS ${REMOTE_PNG_PATH}
        COMMENT "Embedding remote.png as binary resource"
        VERBATIM
    )

    # Add the object file to sources
    list(APPEND SOURCES ${REMOTE_PNG_OBJ})
endif()

if(ANDROID)
    add_library(main SHARED ${SOURCES})
else() # Windows, Linux
    add_library(main STATIC ${SOURCES})
endif()

if (WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    add_definitions(-DGLEW_STATIC)
    target_link_libraries(main ${SDL_STATIC_LDFLAGS} ${SDL_IMAGE_STATIC_LDFLAGS} glew32 opengl32)
elseif(ANDROID OR UNIX)
    # Android: Add ${log-lib} if logging desired
    target_link_libraries(main PRIVATE SDL3::SDL3 SDL3_image::SDL3_image GLESv2)
else()
    # macOS / iOS / iPadOS: these are not cmake builds
endif()
