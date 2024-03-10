cmake_minimum_required(VERSION 3.16)
project(Auto_Vk_Toolkit)

if (MSVC)
    # <ranges> support requires /std:c++latest on MSVC
    set(CMAKE_CXX_STANDARD 23)
else (MSVC)
    set(CMAKE_CXX_STANDARD 20)
endif (MSVC)

include(FetchContent)

# ----------------------- Options -------------------------

set(avk_toolkit_AllowedLibraryTypes INTERFACE SHARED STATIC)
set(avk_toolkit_LibraryType STATIC CACHE STRING
        "The type of library avk_toolkit should be built as. Must be one of ${avk_toolkit_AllowedLibraryTypes}. Default: STATIC")
set_property(CACHE avk_toolkit_LibraryType PROPERTY STRINGS ${avk_toolkit_AllowedLibraryTypes})

if(NOT avk_toolkit_LibraryType IN_LIST avk_toolkit_AllowedLibraryTypes)
    message(FATAL_ERROR "avk_toolkit_LibraryType must be one of ${avk_toolkit_AllowedLibraryTypes}")
endif()

set(avk_toolkit_IncludeScope INTERFACE)
set(avk_toolkit_SourceScope INTERFACE)
if (NOT avk_toolkit_LibraryType STREQUAL "INTERFACE")
    set(avk_toolkit_IncludeScope PUBLIC)
    set(avk_toolkit_SourceScope PRIVATE)
endif (NOT avk_toolkit_LibraryType STREQUAL "INTERFACE")

option(avk_toolkit_ForceAssimpBuild "Force and use a local build of Assimp even if it is installed on the system. (Linux only)" OFF)
option(avk_toolkit_StaticDependencies "Build dependencies as static instead of shared libraries (Assimp & GLFW). (Linux only)" OFF)
if (avk_toolkit_StaticDependencies)
    set(BUILD_SHARED_LIBS OFF)
endif()

option(avk_toolkit_ReleaseDLLsOnly "Use release DLLs for all dependencies of examples. (Windows only)" ON)
option(avk_toolkit_CreateDependencySymlinks "Create symbolic links instead of copying dependencies of examples, i.e. DLLs (Windows only) & assets." ON)

option(avk_toolkit_BuildExamples "Build all examples for Auto-Vk-Toolkit." OFF)
option(avk_toolkit_BuildHelloWorld "Build example: hello_world." OFF)
option(avk_toolkit_BuildFramebuffer "Build example: framebuffer." OFF)
option(avk_toolkit_BuildDynamicRendering "Build example: dynamic_rendering." OFF)
option(avk_toolkit_BuildComputeImageProcessing "Build example: compute_image_processing." OFF)
option(avk_toolkit_BuildMultiInvokeeRendering "Build example: multi_invokee_rendering." OFF)
option(avk_toolkit_BuildModelLoader "Build example: model_loader." OFF)
option(avk_toolkit_BuildStaticMeshlets "Build example: static_meshlets." OFF)
option(avk_toolkit_BuildSkinnedMeshlets "Build example: skinned_meshlets." OFF)
option(avk_toolkit_BuildOrcaLoader "Build example: orca_loader." OFF)
option(avk_toolkit_BuildRayQueryInRayTracingShaders "Build example: ray_query_in_ray_tracing_shaders." OFF)
option(avk_toolkit_BuildRayTracingWithShadowsAndAO "Build example: ray_tracing_with_shadows_and_ao." OFF)
option(avk_toolkit_BuildRayTracingCustomIntersection "Build example: ray_tracing_custom_intersection." OFF)
option(avk_toolkit_BuildTextureCubemap "Build example: texture_cubemap." OFF)
option(avk_toolkit_BuildVertexBuffers "Build example: vertex_buffers." OFF)
option(avk_toolkit_BuildMultipleQueues "Build example: multiple_queues." OFF)
option(avk_toolkit_BuildPresentFromCompute "Build example: present_from_compute." OFF)

if (avk_toolkit_BuildExamples)
    set(avk_toolkit_BuildHelloWorld ON)
    set(avk_toolkit_BuildFramebuffer ON)
    set(avk_toolkit_BuildDynamicRendering ON)
    set(avk_toolkit_BuildComputeImageProcessing ON)
    set(avk_toolkit_BuildMultiInvokeeRendering ON)
    set(avk_toolkit_BuildModelLoader ON)
    set(avk_toolkit_BuildStaticMeshlets ON)
    set(avk_toolkit_BuildSkinnedMeshlets ON)
    set(avk_toolkit_BuildOrcaLoader ON)
    set(avk_toolkit_BuildRayQueryInRayTracingShaders ON)
    set(avk_toolkit_BuildRayTracingWithShadowsAndAO ON)
    set(avk_toolkit_BuildRayTracingCustomIntersection ON)
    set(avk_toolkit_BuildTextureCubemap ON)
    set(avk_toolkit_BuildVertexBuffers ON)
    set(avk_toolkit_BuildMultipleQueues ON)
    set(avk_toolkit_BuildPresentFromCompute ON)
endif()

# ---------------------- Auto-Vk-Toolkit Framework ------------------------

add_library(${PROJECT_NAME} ${avk_toolkit_LibraryType})

# Append MSVC specific compiler flags
if (MSVC)
    target_compile_options(${PROJECT_NAME}
        ${avk_toolkit_IncludeScope} "/bigobj"
        ${avk_toolkit_IncludeScope} "/MP")
endif(MSVC)

# Disable obsolete functionality in ImGui:
# (Can be removed once ImGui has deprecated these for good, and acts as if these were defined.)
target_compile_definitions(${PROJECT_NAME} PUBLIC IMGUI_DISABLE_OBSOLETE_KEYIO IMGUI_DISABLE_OBSOLETE_FUNCTIONS)

## Direct Sources & Includes
# NOTE: We first collect all include directories and source files and add it to our target in the end.
#   We do it like this, because depending on avk_toolkit_LibraryType the scope (INTERFACE, PUBLIC, PRIVATE) changes and we don't
#   want to copy this over and over.
set(avk_toolkit_IncludeDirs
        # external
        external/$<IF:$<BOOL:${UNIX}>,linux,universal>/include

        # Auto-Vk-Toolkit framework files:
        auto_vk_toolkit/include)
set(avk_toolkit_Sources
        # FileWatcher
        external/universal/src/FileWatcher/FileWatcher.cpp
        external/universal/src/FileWatcher/FileWatcherLinux.cpp
        external/universal/src/FileWatcher/FileWatcherOSX.cpp
        external/universal/src/FileWatcher/FileWatcherWin32.cpp

        # Auto-Vk-Toolkit framework files:
        auto_vk_toolkit/src/animation.cpp
        auto_vk_toolkit/src/bezier_curve.cpp
        auto_vk_toolkit/src/camera.cpp
        auto_vk_toolkit/src/catmull_rom_spline.cpp
        auto_vk_toolkit/src/composition.cpp
        auto_vk_toolkit/src/composition_interface.cpp
        auto_vk_toolkit/src/context_generic_glfw.cpp
        auto_vk_toolkit/src/context_vulkan.cpp
        auto_vk_toolkit/src/cp_interpolation.cpp
        auto_vk_toolkit/src/cubic_uniform_b_spline.cpp
        auto_vk_toolkit/src/files_changed_event.cpp
        auto_vk_toolkit/src/fixed_update_timer.cpp
        auto_vk_toolkit/src/imgui_manager.cpp
        auto_vk_toolkit/src/imgui_utils.cpp
        auto_vk_toolkit/src/image_data.cpp
        auto_vk_toolkit/src/input_buffer.cpp
        auto_vk_toolkit/src/log.cpp
        auto_vk_toolkit/src/material_image_helpers.cpp
        auto_vk_toolkit/src/math_utils.cpp
        auto_vk_toolkit/src/meshlet_helpers.cpp
        auto_vk_toolkit/src/model.cpp
        auto_vk_toolkit/src/orca_scene.cpp
        auto_vk_toolkit/src/quadratic_uniform_b_spline.cpp
        auto_vk_toolkit/src/quake_camera.cpp
        auto_vk_toolkit/src/orbit_camera.cpp
        auto_vk_toolkit/src/swapchain_resized_event.cpp
        auto_vk_toolkit/src/transform.cpp
        auto_vk_toolkit/src/timer_globals.cpp
        auto_vk_toolkit/src/updater.cpp
        auto_vk_toolkit/src/varying_update_timer.cpp
        auto_vk_toolkit/src/vk_convenience_functions.cpp
        auto_vk_toolkit/src/window.cpp
        auto_vk_toolkit/src/window_base.cpp)

## Load Dependencies
target_include_directories(${PROJECT_NAME} ${avk_toolkit_IncludeScope}
    ${avk_toolkit_IncludeDirs})
target_sources(${PROJECT_NAME} ${avk_toolkit_SourceScope}
    ${avk_toolkit_Sources})

# Auto-Vk
# TODO: should CMake ever replace the Visual Studio build process this should use cmake/avk.cmake to fetch Auto-Vk
set(avk_UseVMA ON)                   # make sure to add VMA stuff to Auto-Vk's sources
set(avk_LibraryType INTERFACE)       # this is the default, but just in case
add_subdirectory(auto_vk)            # we use the git submodule here as long as we have two separate build systems
target_link_libraries(${PROJECT_NAME} ${avk_toolkit_IncludeScope}
    avk)

# Assimp
include(cmake/assimp.cmake)
if (UNIX)
    target_include_directories(${PROJECT_NAME} ${avk_toolkit_IncludeScope}
        ${assimp_SOURCE_DIR}/include/assimp)
endif()
target_link_libraries(${PROJECT_NAME} ${avk_toolkit_IncludeScope}
    assimp::assimp)

# GLFW
include(cmake/glfw.cmake)
if(UNIX)
    target_include_directories(${PROJECT_NAME} ${avk_toolkit_IncludeScope}
        ${glfw_SOURCE_DIR}/include/GLFW)
endif(UNIX)
target_link_libraries(${PROJECT_NAME} ${avk_toolkit_IncludeScope}
    glfw)

# Vulkan
find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} ${avk_toolkit_IncludeScope}
    Vulkan::Vulkan)

# Dear ImGui
include(cmake/imgui.cmake)
if (UNIX)
    target_include_directories(${PROJECT_NAME} ${avk_toolkit_IncludeScope}
            ${avk_toolkit_imguiIncludeDirectories})
endif(UNIX)
target_sources(${PROJECT_NAME} ${avk_toolkit_SourceScope}
        ${avk_toolkit_imguiSources})

# stb
# NOTE: Windows builds use a modified version of stb (including a small custom DLL).
#   Since this version doesn't work on Linux, the Linux build process generates its own STB shared library.
include(cmake/stb.cmake)
if (UNIX)
    target_link_libraries(${PROJECT_NAME} ${avk_toolkit_IncludeScope}
            stb_shared)
else()
    target_link_libraries(${PROJECT_NAME} ${avk_toolkit_IncludeScope}
        stb)
endif(UNIX)

if (UNIX)
    # pthreads
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} ${avk_toolkit_IncludeScope}
        Threads::Threads)
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # explicitly link dbghelp to debug builds on Windows
    target_link_libraries(${PROJECT_NAME} ${avk_toolkit_IncludeScope}
        dbghelp)
endif (UNIX)

# add avk_toolkit.hpp as precomiled header
target_precompile_headers(${PROJECT_NAME} ${avk_toolkit_IncludeScope} [["auto_vk_toolkit.hpp"]])

# ---------------------- Examples -------------------------

include(cmake/post_build_helper/add_post_build_commands.cmake)

## hello_world
if (avk_toolkit_BuildHelloWorld)
    add_subdirectory(examples/hello_world)
endif()

## framebuffer
if (avk_toolkit_BuildFramebuffer)
    add_subdirectory(examples/framebuffer)
endif()

## dynamic_rendering
if (avk_toolkit_BuildDynamicRendering)
    add_subdirectory(examples/dynamic_rendering)
endif()

## compute_image_processing
if (avk_toolkit_BuildComputeImageProcessing)
    add_subdirectory(examples/compute_image_processing)
endif()

## multi_invokee_rendering
if (avk_toolkit_BuildMultiInvokeeRendering)
    add_subdirectory(examples/multi_invokee_rendering)
endif()

## model_loader
if (avk_toolkit_BuildModelLoader)
    add_subdirectory(examples/model_loader)
endif()

## static_meshlets
if (avk_toolkit_BuildStaticMeshlets)
    add_subdirectory(examples/static_meshlets)
endif()

## skinned_meshlets
if (avk_toolkit_BuildSkinnedMeshlets)
    add_subdirectory(examples/skinned_meshlets)
endif()

## orca_loader
if (avk_toolkit_BuildOrcaLoader)
    add_subdirectory(examples/orca_loader)
endif()

## ray_query_in_ray_tracing_shaders
if (avk_toolkit_BuildRayQueryInRayTracingShaders)
    add_subdirectory(examples/ray_query_in_ray_tracing_shaders)
endif()

## ray_tracing_with_shadows_and_ao
if (avk_toolkit_BuildRayTracingWithShadowsAndAO)
    add_subdirectory(examples/ray_tracing_with_shadows_and_ao)
endif()

## ray_tracing_custom_intersection
if (avk_toolkit_BuildRayTracingCustomIntersection)
    add_subdirectory(examples/ray_tracing_custom_intersection)
endif()

## texture_cubemap
if (avk_toolkit_BuildTextureCubemap)
    add_subdirectory(examples/texture_cubemap)
endif()

## vertex_buffers
if (avk_toolkit_BuildVertexBuffers)
    add_subdirectory(examples/vertex_buffers)
endif()

## multiple_queues
if (avk_toolkit_BuildMultipleQueues)
    add_subdirectory(examples/multiple_queues)
endif()

## present_from_compute
if (avk_toolkit_BuildPresentFromCompute)
    add_subdirectory(examples/present_from_compute)
endif()
