cmake_minimum_required(VERSION 3.11)

include(FetchContent)

if(UNIX)
    FetchContent_Declare(
            stb
            GIT_REPOSITORY      https://github.com/nothings/stb.git
            GIT_TAG             master
    )

    FetchContent_GetProperties(stb)
    if(NOT stb_POPULATED)
        FetchContent_Populate(stb)
    endif()
else()
    set(gvk_STBReleaseDLLPath "${PROJECT_SOURCE_DIR}/external/release/bin/x64/stb.dll")
    set(gvk_STBDebugDLLPath "${PROJECT_SOURCE_DIR}/external/debug/bin/x64/stb.dll")
    set(gvk_STBReleaseLIBPath "${PROJECT_SOURCE_DIR}/external/release/lib/x64/stb.lib")
    set(gvk_STBDebugLIBPath "${PROJECT_SOURCE_DIR}/external/debug/lib/x64/stb.lib")

    add_library(stb SHARED IMPORTED)
    set_target_properties(stb PROPERTIES
        IMPORTED_IMPLIB         "${gvk_STBReleaseLIBPath}"
        IMPORTED_IMPLIB_DEBUG   "${gvk_STBDebugLIBPath}"
        IMPORTED_LOCATION       "${gvk_STBReleaseDLLPath}"
        IMPORTED_LOCATION_DEBUG "${gvk_STBDebugDLLPath}"
        IMPORTED_CONFIGURATIONS "RELEASE;DEBUG")
endif(UNIX)
