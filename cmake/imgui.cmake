cmake_minimum_required(VERSION 3.11)

include(FetchContent)

if(UNIX)
    FetchContent_Declare(
            imgui
            GIT_REPOSITORY      https://github.com/ocornut/imgui
            GIT_TAG             v1.89.5
    )

    FetchContent_GetProperties(imgui)
    if(NOT imgui_POPULATED)
        FetchContent_Populate(imgui)
    endif()

    set(avk_toolkit_imguiIncludeDirectories
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends)

    set(avk_toolkit_imguiSources
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp)
else()
    set(avk_toolkit_imguiSources
            ${PROJECT_SOURCE_DIR}/external/universal/src/imgui.cpp
            ${PROJECT_SOURCE_DIR}/external/universal/src/imgui_demo.cpp
            ${PROJECT_SOURCE_DIR}/external/universal/src/imgui_draw.cpp
            ${PROJECT_SOURCE_DIR}/external/universal/src/imgui_impl_glfw.cpp
            ${PROJECT_SOURCE_DIR}/external/universal/src/imgui_impl_vulkan.cpp
            ${PROJECT_SOURCE_DIR}/external/universal/src/imgui_tables.cpp
            ${PROJECT_SOURCE_DIR}/external/universal/src/imgui_widgets.cpp)
endif(UNIX)
