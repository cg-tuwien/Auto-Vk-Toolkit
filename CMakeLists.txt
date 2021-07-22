cmake_minimum_required(VERSION 3.13)
project(Gears_Vk)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

# TODO: add option gvk_LibraryType (STATIC, SHARED) and decide on default

add_library(${PROJECT_NAME} SHARED)
target_include_directories(${PROJECT_NAME} PUBLIC
        framework/include)
target_sources(${PROJECT_NAME} PRIVATE
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

# TODO: this unholy mix of CMake and VS build system should come to an end
# TODO: how should dependencies be handled? conan?

# add external include directories
target_include_directories(${PROJECT_NAME} PUBLIC
        external/linux/include)

# Auto-Vk
set(avk_UseVMA ON)
include(cmake/avk.cmake)
target_link_libraries(${PROJECT_NAME} PUBLIC avk)

# stb
include(cmake/stb.cmake)
target_include_directories(${PROJECT_NAME} PRIVATE ${stb_SOURCE_DIR})

# Assimp
include(cmake/assimp.cmake)
target_include_directories(${PROJECT_NAME} PUBLIC ${assimp_SOURCE_DIR}/include/assimp)
target_link_libraries(${PROJECT_NAME} PUBLIC assimp::assimp)

# GLFW
include(cmake/glfw.cmake)
target_include_directories(${PROJECT_NAME} PUBLIC ${glfw_SOURCE_DIR}/include/GLFW)
target_link_libraries(${PROJECT_NAME} PUBLIC glfw)

# IMGUI
# TODO: use official version as soon as this is fixed (hopefully v1.82): https://github.com/ocornut/imgui/pull/914
#include(cmake/imgui.cmake)
#target_include_directories(${PROJECT_NAME} PUBLIC ${imgui_SOURCE_DIR})
#target_link_libraries(${PROJECT_NAME} PUBLIC imgui)
# NOTE: a fixed version of imgui is now part of this repo
# TODO: are all of those source needed?
target_sources(${PROJECT_NAME} PRIVATE
        external/universal/include/imgui/imconfig.h
        external/universal/include/imgui/imgui.h
        external/universal/include/imgui/imgui_impl_glfw.h
        external/universal/include/imgui/imgui_impl_vulkan.h
        external/universal/include/imgui/imgui_internal.h
        external/universal/include/imgui/imstb_rectpack.h
        external/universal/include/imgui/imstb_textedit.h
        external/universal/include/imgui/imstb_truetype.h

        external/universal/src/imgui.cpp
        external/universal/src/imgui_demo.cpp
        external/universal/src/imgui_draw.cpp
        external/universal/src/imgui_impl_glfw.cpp
        external/universal/src/imgui_impl_vulkan.cpp
        external/universal/src/imgui_tables.cpp
        external/universal/src/imgui_widgets.cpp)

# FileWatcher
# TODO: would be cleaner to just create a separate repository / fork, since this repo seems to be unmaintained
# there were some changes in the source files in this repo only...
#include(cmake/simplefilewatcher.cmake)
#target_include_directories(${PROJECT_NAME} PUBLIC ${simplefilewatcher_SOURCE_DIR}/include)
#target_link_libraries(${PROJECT_NAME} PUBLIC simplefilewatcher)
target_sources(${PROJECT_NAME} PRIVATE
        external/universal/src/FileWatcher/FileWatcher.cpp
        external/universal/src/FileWatcher/FileWatcherLinux.cpp
        external/universal/src/FileWatcher/FileWatcherOSX.cpp
        external/universal/src/FileWatcher/FileWatcherWin32.cpp)

# Vulkan
find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Vulkan::Vulkan)

if (UNIX)
    # pthreads
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)
endif (UNIX)


# TODO: build all examples
# TODO: only build examples (if option is set)

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
