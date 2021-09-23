cmake_minimum_required(VERSION 3.16)
project(Gears_Vk)

if (MSVC)
    # <ranges> support requires /std:c++latest on MSVC
    set(CMAKE_CXX_STANDARD 23)
else (MSVC)
    set(CMAKE_CXX_STANDARD 20)
endif (MSVC)

include(FetchContent)

# ----------------------- Options -------------------------

set(gvk_AllowedLibraryTypes INTERFACE SHARED STATIC)
set(gvk_LibraryType STATIC CACHE STRING
        "The type of library gvk should be built as. Must be one of ${gvk_AllowedLibraryTypes}. Default: STATIC")
set_property(CACHE gvk_LibraryType PROPERTY STRINGS ${gvk_AllowedLibraryTypes})

if(NOT gvk_LibraryType IN_LIST gvk_AllowedLibraryTypes)
    message(FATAL_ERROR "gvk_LibraryType must be one of ${gvk_AllowedLibraryTypes}")
endif()

set(gvk_IncludeScope INTERFACE)
set(gvk_SourceScope INTERFACE)
if (NOT gvk_LibraryType STREQUAL "INTERFACE")
    set(gvk_IncludeScope PUBLIC)
    set(gvk_SourceScope PRIVATE)
endif (NOT gvk_LibraryType STREQUAL "INTERFACE")

option(gvk_ForceAssimpBuild "Force and use a local build of Assimp even if it is installed on the system. (Linux only)" OFF)
option(gvk_StaticDependencies "Build dependencies as static instead of shared libraries (Assimp & GLFW). (Linux only)" OFF)
if (gvk_StaticDependencies)
    set(BUILD_SHARED_LIBS OFF)
endif()

option(gvk_ReleaseDLLsOnly "Use release DLLs for all dependencies of examples. (Windows only)" ON)
option(gvk_CreateDependencySymlinks "Create symbolic links instead of copying dependencies of examples, i.e. DLLs (Windows only) & assets." ON)

option(gvk_BuildExamples "Build all examples for Gears-Vk." OFF)
option(gvk_BuildHelloWorld "Build example: hello_world." OFF)
option(gvk_BuildFramebuffer "Build example: framebuffer." OFF)
option(gvk_BuildComputeImageProcessing "Build example: compute_image_processing." OFF)
option(gvk_BuildMultiInvokeeRendering "Build example: multi_invokee_rendering." OFF)
option(gvk_BuildModelLoader "Build example: model_loader." OFF)
option(gvk_BuildOrcaLoader "Build example: orca_loader." OFF)
option(gvk_BuildRayQueryInRayTracingShaders "Build example: ray_query_in_ray_tracing_shaders." OFF)
option(gvk_BuildRayTracingWithShadowsAndAO "Build example: ray_tracing_with_shadows_and_ao." OFF)
option(gvk_BuildRayTracingCustomIntersection "Build example: ray_tracing_custom_intersection." OFF)
option(gvk_BuildTextureCubemap "Build example: texture_cubemap." OFF)
option(gvk_BuildVertexBuffers "Build example: vertex_buffers." OFF)

if (gvk_BuildExamples)
    set(gvk_BuildHelloWorld ON)
    set(gvk_BuildFramebuffer ON)
    set(gvk_BuildComputeImageProcessing ON)
    set(gvk_BuildMultiInvokeeRendering ON)
    set(gvk_BuildModelLoader ON)
    set(gvk_BuildOrcaLoader ON)
    set(gvk_BuildRayQueryInRayTracingShaders ON)
    set(gvk_BuildRayTracingWithShadowsAndAO ON)
    set(gvk_BuildRayTracingCustomIntersection ON)
    set(gvk_BuildTextureCubemap ON)
    set(gvk_BuildVertexBuffers ON)
endif()

# ---------------------- Framework ------------------------

add_library(${PROJECT_NAME} ${gvk_LibraryType})

# Append MSVC specific compiler flags
if (MSVC)
    target_compile_options(${PROJECT_NAME}
        ${gvk_IncludeScope} "/bigobj"
        ${gvk_IncludeScope} "/MP")
endif(MSVC)

## Direct Sources & Includes
# NOTE: We first collect all include directories and source files and add it to our target in the end.
#   We do it like this, because depending on gvk_LibraryType the scope (INTERFACE, PUBLIC, PRIVATE) changes and we don't
#   want to copy this over and over.
set(gvk_IncludeDirs
        # external
        external/$<IF:$<BOOL:${UNIX}>,linux,universal>/include

        # framework
        framework/include)
set(gvk_Sources
        # FileWatcher
        external/universal/src/FileWatcher/FileWatcher.cpp
        external/universal/src/FileWatcher/FileWatcherLinux.cpp
        external/universal/src/FileWatcher/FileWatcherOSX.cpp
        external/universal/src/FileWatcher/FileWatcherWin32.cpp

        # ImGui
        external/universal/src/imgui.cpp
        external/universal/src/imgui_demo.cpp
        external/universal/src/imgui_draw.cpp
        external/universal/src/imgui_impl_glfw.cpp
        external/universal/src/imgui_impl_vulkan.cpp
        external/universal/src/imgui_tables.cpp
        external/universal/src/imgui_widgets.cpp

        # framework
        framework/src/animation.cpp
        framework/src/bezier_curve.cpp
        framework/src/camera.cpp
        framework/src/catmull_rom_spline.cpp
        framework/src/cgb_exceptions.cpp
        framework/src/composition.cpp
        framework/src/composition_interface.cpp
        framework/src/context_generic_glfw.cpp
        framework/src/context_vulkan.cpp
        framework/src/cp_interpolation.cpp
        framework/src/cubic_uniform_b_spline.cpp
        framework/src/files_changed_event.cpp
        framework/src/fixed_update_timer.cpp
        framework/src/imgui_manager.cpp
        framework/src/imgui_utils.cpp
        framework/src/image_data.cpp
        framework/src/input_buffer.cpp
        framework/src/log.cpp
        framework/src/material_image_helpers.cpp
        framework/src/math_utils.cpp
        framework/src/meshlet_helpers.cpp
        framework/src/model.cpp
        framework/src/orca_scene.cpp
        framework/src/quadratic_uniform_b_spline.cpp
        framework/src/quake_camera.cpp
        framework/src/swapchain_resized_event.cpp
        framework/src/transform.cpp
        framework/src/updater.cpp
        framework/src/varying_update_timer.cpp
        framework/src/vk_convenience_functions.cpp
        framework/src/window.cpp
        framework/src/window_base.cpp)

## Load Dependencies
target_include_directories(${PROJECT_NAME} ${gvk_IncludeScope}
    ${gvk_IncludeDirs})
target_sources(${PROJECT_NAME} ${gvk_SourceScope}
    ${gvk_Sources})

# Auto-Vk
# TODO: should CMake ever replace the Visual Studio build process this should use cmake/avk.cmake to fetch Auto-Vk
set(avk_UseVMA ON)                   # make sure to add VMA stuff to Auto-Vk's sources
set(avk_LibraryType INTERFACE)       # this is the default, but just in case
add_subdirectory(auto_vk)            # we use the git submodule here as long as we have two separate build systems
target_link_libraries(${PROJECT_NAME} ${gvk_IncludeScope}
    avk)

# Assimp
include(cmake/assimp.cmake)
if (UNIX)
    target_include_directories(${PROJECT_NAME} ${gvk_IncludeScope}
        ${assimp_SOURCE_DIR}/include/assimp)
endif()
target_link_libraries(${PROJECT_NAME} ${gvk_IncludeScope}
    assimp::assimp)

# GLFW
include(cmake/glfw.cmake)
if(UNIX)
    target_include_directories(${PROJECT_NAME} ${gvk_IncludeScope}
        ${glfw_SOURCE_DIR}/include/GLFW)
endif(UNIX)
target_link_libraries(${PROJECT_NAME} ${gvk_IncludeScope}
    glfw)

# Vulkan
find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} ${gvk_IncludeScope}
    Vulkan::Vulkan)

# stb
# NOTE: Windows builds use a modified version of stb (including a small custom DLL).
#   Since this version doesn't work on Linux, the Linux build process generates its own STB shared library.
include(cmake/stb.cmake)
if (UNIX)
    target_link_libraries(${PROJECT_NAME} ${gvk_IncludeScope}
            stb_shared)
else()
    target_link_libraries(${PROJECT_NAME} ${gvk_IncludeScope}
        stb)
endif(UNIX)

if (UNIX)
    # pthreads
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} ${gvk_IncludeScope}
        Threads::Threads)
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # explicitly link dbghelp to debug builds on Windows
    target_link_libraries(${PROJECT_NAME} ${gvk_IncludeScope}
        dbghelp)
endif (UNIX)

# add gvk.hpp as precomiled header
target_precompile_headers(${PROJECT_NAME} ${gvk_IncludeScope} [["gvk.hpp"]])

# ---------------------- Examples -------------------------

include(cmake/post_build_helper/add_post_build_commands.cmake)

## hello_world
if (gvk_BuildHelloWorld)
    add_subdirectory(examples/hello_world)
endif()

## framebuffer
if (gvk_BuildFramebuffer)
    add_subdirectory(examples/framebuffer)
endif()

## compute_image_processing
if (gvk_BuildComputeImageProcessing)
    add_subdirectory(examples/compute_image_processing)
endif()

## multi_invokee_rendering
if (gvk_BuildMultiInvokeeRendering)
    add_subdirectory(examples/multi_invokee_rendering)
endif()

## model_loader
if (gvk_BuildModelLoader)
    add_subdirectory(examples/model_loader)
endif()

## orca_loader
if (gvk_BuildOrcaLoader)
    add_subdirectory(examples/orca_loader)
endif()

## ray_query_in_ray_tracing_shaders
if (gvk_BuildRayQueryInRayTracingShaders)
    add_subdirectory(examples/ray_query_in_ray_tracing_shaders)
endif()

## ray_tracing_with_shadows_and_ao
if (gvk_BuildRayTracingWithShadowsAndAO)
    add_subdirectory(examples/ray_tracing_with_shadows_and_ao)
endif()

## ray_tracing_custom_intersection
if (gvk_BuildRayTracingCustomIntersection)
    add_subdirectory(examples/ray_tracing_custom_intersection)
endif()

## texture_cubemap
if (gvk_BuildTextureCubemap)
    add_subdirectory(examples/texture_cubemap)
endif()

## vertex_buffers
if (gvk_BuildVertexBuffers)
    add_subdirectory(examples/vertex_buffers)
endif()
