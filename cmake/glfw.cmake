cmake_minimum_required(VERSION 3.14)

include(FetchContent)

if(UNIX)
    set(GLFW_BUILD_EXAMPLES OFF)
    set(GLFW_BUILD_TESTS OFF)
    set(GLFW_BUILD_DOCS OFF)
    set(GLFW_INSTALL OFF)

    FetchContent_Declare(
            glfw
            GIT_REPOSITORY      https://github.com/glfw/glfw.git
            GIT_TAG             3.3.2
    )

    FetchContent_MakeAvailable(glfw)
else()
    set(avk_toolkit_GLFWReleaseLIBPath "${PROJECT_SOURCE_DIR}/external/release/lib/x64/glfw3.lib")
    set(avk_toolkit_GLFWDebugLIBPath "${PROJECT_SOURCE_DIR}/external/debug/lib/x64/glfw3.lib")

    add_library(glfw STATIC IMPORTED)
    set_target_properties(glfw PROPERTIES
        IMPORTED_LOCATION       "${avk_toolkit_GLFWReleaseLIBPath}"
        IMPORTED_LOCATION_DEBUG "${avk_toolkit_GLFWDebugLIBPath}"
        IMPORTED_CONFIGURATIONS "RELEASE;DEBUG")
endif(UNIX)