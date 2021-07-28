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

# TODO: add option - shared or static Assimp & GLFW
#  set BUILD_SHARED_LIBS based on a gvk specific option

# TODO: add option - build compute_image_processing
# TODO: add option - build framebuffer
# TODO: add option - build hello_world
# TODO: add option - build multi_invokee_rendering
# TODO: add option - build orca_loader
# TODO: add option - build ray_query_in_ray_tracing_shaders
# TODO: add option - build ray_tracing_with_shadows_and_ao
# TODO: add option - build texture_cubemap
# TODO: add option - build vertex_buffers

# ---------------------- Framework ------------------------

add_library(${PROJECT_NAME} ${gvk_LibraryType})

## Direct Sources & Includes
# NOTE: We first collect all include directories and source files and add it to our target in the end.
#   We do it like this, because depending on gvk_LibraryType the scope (INTERFACE, PUBLIC, PRIVATE) changes and we don't
#   want to copy this over and over.
set(gvk_IncludeDirs
        # external
        external/$<IF:${UNIX},linux,universal>/include

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

# TODO: only build examples if option is set

# hello_world
add_executable(hello_world
        examples/hello_world/source/hello_world.cpp)
target_include_directories(hello_world PRIVATE ${PROJECT_NAME})
target_link_libraries(hello_world PRIVATE ${PROJECT_NAME})

# this is done by the post build helper on windows
file(COPY examples/hello_world/shaders DESTINATION ${CMAKE_BINARY_DIR})


# model_loader
add_executable(model_loader
        examples/model_loader/source/model_loader.cpp)
target_include_directories(model_loader PRIVATE ${PROJECT_NAME})
target_link_libraries(model_loader PRIVATE ${PROJECT_NAME})

# this is done by the post build helper on windows
file(COPY examples/model_loader/shaders DESTINATION ${CMAKE_BINARY_DIR})
file(COPY assets/3rd_party/models/sponza/sponza_structure.obj DESTINATION ${CMAKE_BINARY_DIR}/assets)

# compute_image_processing
add_executable(compute_image_processing
        examples/compute_image_processing/source/compute_image_processing.cpp)
target_include_directories(compute_image_processing PRIVATE ${PROJECT_NAME})
target_link_libraries(compute_image_processing PRIVATE ${PROJECT_NAME})

# this is done by the post build helper on windows
file(COPY examples/compute_image_processing/shaders DESTINATION ${CMAKE_BINARY_DIR})
file(COPY assets/3rd_party/models/sponza/textures/lion.png DESTINATION ${CMAKE_BINARY_DIR}/assets)

# orca_loader
add_executable(orca_loader
        examples/orca_loader/source/orca_loader.cpp)
target_include_directories(orca_loader PRIVATE
        examples/orca_loader/source)
target_include_directories(orca_loader PRIVATE ${PROJECT_NAME})
target_link_libraries(orca_loader PRIVATE ${PROJECT_NAME})

# TODO: invoke post build helper on windows?
# TODO: port post build helper to linux
# TODO: compile shaders before copying them
# TODO: just add a script that does this or port post build helper (is there more to it than copying assets?)

## general assets stuff
# this is done be the post build helper on windows
file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})
#file(COPY assets/3rd_party/models/sponza/textures DESTINATION ${CMAKE_BINARY_DIR}/assets)
