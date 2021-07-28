cmake_minimum_required(VERSION 3.13)
project(Gears_Vk)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

# ----------------------- Options -------------------------

set(gvk_AllowedLibraryTypes INTERFACE SHARED STATIC)
set(gvk_LibraryType INTERFACE CACHE STRING
        "The type of library gvk should be built as. Must be one of ${gvk_AllowedLibraryTypes}. Default: INTERFACE")
set_property(CACHE gvk_LibraryType PROPERTY STRINGS ${gvk_AllowedLibraryTypes})

if(NOT gvk_LibraryType IN_LIST gvk_AllowedLibraryTypes)
    message(FATAL_ERROR "gvk_LibraryType must be one of ${gvk_AllowedLibraryTypes}")
endif()

option(gvk_StaticDependencies "Build dependencies as static instead of shared libraries (Assimp & GLFW)." OFF)
if (gvk_StaticDependencies)
    set(BUILD_SHARED_LIBS OFF)
endif()

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

set(gvk_BuildAnyExample OFF)
if (gvk_BuildHelloWorld OR
        gvk_BuildFramebuffer OR
        gvk_BuildComputeImageProcessing OR
        gvk_BuildMultiInvokeeRendering OR
        gvk_BuildModelLoader OR
        gvk_BuildOrcaLoader OR
        gvk_BuildRayQueryInRayTracingShaders OR
        gvk_BuildRayTracingWithShadowsAndAO OR
        gvk_BuildRayTracingCustomIntersection OR
        gvk_BuildTextureCubemap OR
        gvk_BuildVertexBuffers)
    set(gvk_BuildAnyExample ON)
endif()

# ---------------------- Framework ------------------------

add_library(${PROJECT_NAME} ${gvk_LibraryType})

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
        framework/src/window_base.cpp
        #framework/src/window_vulkan.cpp # is this a cg_base-only thing - remove?
        )

## Load Dependencies
# NOTE: All dependencies are linked further down.

# Auto-Vk
set(avk_UseVMA ON)                   # make sure to add VMA stuff to Auto-Vk's sources
add_compile_definitions(AVK_USE_VMA) # there is something wrong with the order of includes(?), so to be sure we define AVK_USE_VMA here
set(avk_LibraryType INTERFACE)       # this is the default, but just in case
add_subdirectory(auto_vk)            # we use the git submodule here as long as we have two separate build systems

# Assimp
include(cmake/assimp.cmake)

# GLFW
include(cmake/glfw.cmake)

# Vulkan
find_package(Vulkan REQUIRED)

if (UNIX)
    # stb
    # NOTE: the stb in external/universal/include has some fixes for Windows which break compatibility with gcc, so we
    #   need to fetch it from the stb github repo.
    include(cmake/stb.cmake)
    list(APPEND gvk_IncludeDirs ${stb_SOURCE_DIR})

    # pthreads
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
endif (UNIX)

if(NOT gvk_LibraryType STREQUAL "INTERFACE")
    target_include_directories(${PROJECT_NAME} PUBLIC
            ${gvk_IncludeDirs})
    target_sources(${PROJECT_NAME} PRIVATE
            ${gvk_Sources})

    # Auto-Vk
    target_link_libraries(${PROJECT_NAME} PUBLIC
            avk)

    # Assimp
    target_include_directories(${PROJECT_NAME} PUBLIC
            ${assimp_SOURCE_DIR}/include/assimp)
    target_link_libraries(${PROJECT_NAME} PUBLIC
            assimp::assimp)

    # GLFW
    target_include_directories(${PROJECT_NAME} PUBLIC
            ${glfw_SOURCE_DIR}/include/GLFW)
    target_link_libraries(${PROJECT_NAME} PUBLIC
            glfw)

    # Vulkan
    target_link_libraries(${PROJECT_NAME} PUBLIC
            Vulkan::Vulkan)

    if (UNIX)
        # pthreads
        target_link_libraries(${PROJECT_NAME} PUBLIC
                Threads::Threads)
    endif (UNIX)
else()
    target_include_directories(${PROJECT_NAME} INTERFACE
            ${gvk_IncludeDirs})
    target_sources(${PROJECT_NAME} INTERFACE
            ${gvk_Sources})

    # Auto-Vk
    target_link_libraries(${PROJECT_NAME} INTERFACE
            avk)

    # Assimp
    target_include_directories(${PROJECT_NAME} INTERFACE
            ${assimp_SOURCE_DIR}/include/assimp)
    target_link_libraries(${PROJECT_NAME} INTERFACE
            assimp::assimp)

    # GLFW
    target_include_directories(${PROJECT_NAME} INTERFACE
            ${glfw_SOURCE_DIR}/include/GLFW)
    target_link_libraries(${PROJECT_NAME} INTERFACE
            glfw)

    # Vulkan
    target_link_libraries(${PROJECT_NAME} INTERFACE
            Vulkan::Vulkan)

    if (UNIX)
        # pthreads
        target_link_libraries(${PROJECT_NAME} INTERFACE
                Threads::Threads)
    endif (UNIX)
endif()


# ---------------------- Examples -------------------------

# If we build any example we just copy the assets over to the binary dir.
if (gvk_BuildAnyExample)
    file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})
endif()

## hello_world
if (gvk_BuildHelloWorld)
    add_executable(hello_world
            examples/hello_world/source/hello_world.cpp)
    target_include_directories(hello_world PRIVATE ${PROJECT_NAME})
    target_link_libraries(hello_world PRIVATE ${PROJECT_NAME})

    file(COPY examples/hello_world/shaders DESTINATION ${CMAKE_BINARY_DIR})
endif()

## framebuffer
if (gvk_BuildFramebuffer)
    add_executable(framebuffer
            examples/framebuffer/source/framebuffer.cpp)
    target_include_directories(framebuffer PRIVATE ${PROJECT_NAME})
    target_link_libraries(framebuffer PRIVATE ${PROJECT_NAME})

    file(COPY examples/framebuffer/shaders DESTINATION ${CMAKE_BINARY_DIR})
endif()

## compute_image_processing
if (gvk_BuildComputeImageProcessing)
    add_executable(compute_image_processing
            examples/compute_image_processing/source/compute_image_processing.cpp)
    target_include_directories(compute_image_processing PRIVATE ${PROJECT_NAME})
    target_link_libraries(compute_image_processing PRIVATE ${PROJECT_NAME})

    file(COPY examples/compute_image_processing/shaders DESTINATION ${CMAKE_BINARY_DIR})
endif()

## multi_invokee_rendering
if (gvk_BuildMultiInvokeeRendering)
    add_executable(multi_invokee_rendering
            examples/multi_invokee_rendering/source/multi_invokee_rendering.cpp)
    target_include_directories(multi_invokee_rendering PRIVATE ${PROJECT_NAME})
    target_link_libraries(multi_invokee_rendering PRIVATE ${PROJECT_NAME})

    file(COPY examples/multi_invokee_rendering/shaders DESTINATION ${CMAKE_BINARY_DIR})
endif()

## model_loader
if (gvk_BuildModelLoader)
    add_executable(model_loader
            examples/model_loader/source/model_loader.cpp)
    target_include_directories(model_loader PRIVATE
            examples/model_loader/source)
    target_include_directories(model_loader PRIVATE ${PROJECT_NAME})
    target_link_libraries(model_loader PRIVATE ${PROJECT_NAME})

    file(COPY examples/model_loader/shaders DESTINATION ${CMAKE_BINARY_DIR})
endif()

## orca_loader
if (gvk_BuildOrcaLoader)
    add_executable(orca_loader
            examples/orca_loader/source/orca_loader.cpp)
    target_include_directories(orca_loader PRIVATE
            examples/orca_loader/source)
    target_include_directories(orca_loader PRIVATE ${PROJECT_NAME})
    target_link_libraries(orca_loader PRIVATE ${PROJECT_NAME})

    file(COPY examples/orca_loader/shaders DESTINATION ${CMAKE_BINARY_DIR})
endif()

## ray_query_in_ray_tracing_shaders
if (gvk_BuildRayQueryInRayTracingShaders)
    add_executable(ray_query_in_ray_tracing_shaders
            examples/ray_query_in_ray_tracing_shaders/source/ray_query_in_ray_tracing_shaders.cpp)
    target_include_directories(ray_query_in_ray_tracing_shaders PRIVATE
            examples/ray_query_in_ray_tracing_shaders/source)
    target_include_directories(ray_query_in_ray_tracing_shaders PRIVATE ${PROJECT_NAME})
    target_link_libraries(ray_query_in_ray_tracing_shaders PRIVATE ${PROJECT_NAME})

    file(COPY examples/ray_query_in_ray_tracing_shaders/shaders DESTINATION ${CMAKE_BINARY_DIR})
endif()

## ray_tracing_with_shadows_and_ao
if (gvk_BuildRayTracingWithShadowsAndAO)
    add_executable(ray_tracing_with_shadows_and_ao
            examples/ray_tracing_with_shadows_and_ao/source/ray_tracing_with_shadows_and_ao.cpp)
    target_include_directories(ray_tracing_with_shadows_and_ao PRIVATE ${PROJECT_NAME})
    target_link_libraries(ray_tracing_with_shadows_and_ao PRIVATE ${PROJECT_NAME})

    file(COPY examples/ray_tracing_with_shadows_and_ao/shaders DESTINATION ${CMAKE_BINARY_DIR})
endif()

## ray_tracing_custom_intersection
if (gvk_BuildRayTracingCustomIntersection)
    add_executable(ray_tracing_custom_intersection
            examples/ray_tracing_custom_intersection/source/ray_tracing_custom_intersection.cpp)
    target_include_directories(ray_tracing_custom_intersection PRIVATE
            examples/ray_tracing_custom_intersection/source)
    target_include_directories(ray_tracing_custom_intersection PRIVATE ${PROJECT_NAME})
    target_link_libraries(ray_tracing_custom_intersection PRIVATE ${PROJECT_NAME})

    file(COPY examples/ray_tracing_custom_intersection/shaders DESTINATION ${CMAKE_BINARY_DIR})
endif()

## texture_cubemap
if (gvk_BuildTextureCubemap)
    add_executable(texture_cubemap
            examples/texture_cubemap/source/texture_cubemap.cpp)
    target_include_directories(texture_cubemap PRIVATE ${PROJECT_NAME})
    target_link_libraries(texture_cubemap PRIVATE ${PROJECT_NAME})

    file(COPY examples/texture_cubemap/shaders DESTINATION ${CMAKE_BINARY_DIR})
endif()

## vertex_buffers
if (gvk_BuildVertexBuffers)
    add_executable(vertex_buffers
            examples/vertex_buffers/source/vertex_buffers.cpp)
    target_include_directories(vertex_buffers PRIVATE ${PROJECT_NAME})
    target_link_libraries(vertex_buffers PRIVATE ${PROJECT_NAME})

    file(COPY examples/vertex_buffers/shaders DESTINATION ${CMAKE_BINARY_DIR})
endif()
