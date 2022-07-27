cmake_minimum_required(VERSION 3.11)

include(FetchContent)

if(UNIX)
    FetchContent_Declare(
            stb
            GIT_REPOSITORY      https://github.com/nothings/stb.git
            GIT_TAG             08e89524f693651819c4de2a29685b36301a08b1 # the commit after that removes stb_perlin.h
    )

    FetchContent_GetProperties(stb)
    if(NOT stb_POPULATED)
        FetchContent_Populate(stb)
    endif()

    if (NOT EXISTS ${stb_SOURCE_DIR}/stb_lib.cpp)
        file(WRITE ${stb_SOURCE_DIR}/stb_lib.cpp
            "#include \"stb_image.h\"
#include \"stb_image_resize.h\"
#include \"stb_image_write.h\"
#include \"stb_dxt.h\"
#include \"stb_perlin.h\"

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#define STB_IMAGE_IMPLEMENTATION
#include \"stb_image.h\"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include \"stb_image_resize.h\"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include \"stb_image_write.h\"

#define STB_DXT_IMPLEMENTATION
#include \"stb_dxt.h\"

#define STB_PERLIN_IMPLEMENTATION
#include \"stb_perlin.h\"
"
                )
    endif (NOT EXISTS ${stb_SOURCE_DIR}/stb_lib.cpp)

    add_library(stb_shared SHARED)
    target_include_directories(stb_shared PUBLIC
            ${stb_SOURCE_DIR})
    target_sources(stb_shared PRIVATE
            ${stb_SOURCE_DIR}/stb_lib.cpp)
else()
    set(avk_toolkit_STBReleaseDLLPath "${PROJECT_SOURCE_DIR}/external/release/bin/x64/stb.dll")
    set(avk_toolkit_STBDebugDLLPath "${PROJECT_SOURCE_DIR}/external/debug/bin/x64/stb.dll")
    set(avk_toolkit_STBReleaseLIBPath "${PROJECT_SOURCE_DIR}/external/release/lib/x64/stb.lib")
    set(avk_toolkit_STBDebugLIBPath "${PROJECT_SOURCE_DIR}/external/debug/lib/x64/stb.lib")

    add_library(stb SHARED IMPORTED)
    set_target_properties(stb PROPERTIES
        IMPORTED_IMPLIB         "${avk_toolkit_STBReleaseLIBPath}"
        IMPORTED_IMPLIB_DEBUG   "${avk_toolkit_STBDebugLIBPath}"
        IMPORTED_LOCATION       "${avk_toolkit_STBReleaseDLLPath}"
        IMPORTED_LOCATION_DEBUG "${avk_toolkit_STBDebugDLLPath}"
        IMPORTED_CONFIGURATIONS "RELEASE;DEBUG")
endif(UNIX)
