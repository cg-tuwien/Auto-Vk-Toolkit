cmake_minimum_required(VERSION 3.14)

include(FetchContent)

if(UNIX)
    if (NOT gvk_ForceAssimpBuild)
        find_package(assimp)
    endif (NOT gvk_ForceAssimpBuild)
    if (NOT assimp_FOUND OR gvk_ForceAssimpBuild)
        set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
        set(ASSIMP_BUILD_TESTS OFF)
        set(INJECT_DEBUG_POSTFIX OFF)

        FetchContent_Declare(
                assimp
                GIT_REPOSITORY      https://github.com/assimp/assimp.git
                GIT_TAG             v5.0.1
        )

        FetchContent_MakeAvailable(assimp)
    else (NOT assimp_FOUND OR gvk_ForceAssimpBuild)
        # there is some issue with libassimp-dev in the GitHub workflows on Ubuntu
        # see:
        #  - https://github.com/cg-tuwien/Gears-Vk/runs/3432527652?check_suite_focus=true#step:4:80
        #  - https://bugs.launchpad.net/ubuntu/+source/assimp/+bug/1882427
        # the following work-around should fix this
        # source: https://github.com/robotology/idyntree/issues/693#issuecomment-640216067
        get_property(assimp_INTERFACE_INCLUDE_DIRECTORIES
                TARGET assimp::assimp
                PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
        if(assimp_INTERFACE_INCLUDE_DIRECTORIES MATCHES "/usr/lib/include")
            string(REPLACE "/usr/lib/include" "/usr/include" assimp_INTERFACE_INCLUDE_DIRECTORIES "${assimp_INTERFACE_INCLUDE_DIRECTORIES}")
            set_property(TARGET assimp::assimp
                    PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                    "${assimp_INTERFACE_INCLUDE_DIRECTORIES}")
            get_property(assimp_LOCATION_RELEASE
                    TARGET assimp::assimp
                    PROPERTY LOCATION_RELEASE)
            set_property(TARGET assimp::assimp
                    PROPERTY IMPORTED_LOCATION
                    "${assimp_LOCATION_RELEASE}")
        endif()
    endif (NOT assimp_FOUND OR gvk_ForceAssimpBuild)
else()
    set(gvk_AssimpReleaseDLLPath "${PROJECT_SOURCE_DIR}/external/release/bin/x64/assimp-vc140-mt.dll")
    set(gvk_AssimpDebugDLLPath "${PROJECT_SOURCE_DIR}/external/debug/bin/x64/assimp-vc140-mt.dll")
    set(gvk_AssimpReleaseLIBPath "${PROJECT_SOURCE_DIR}/external/release/lib/x64/assimp-vc140-mt.lib")
    set(gvk_AssimpDebugLIBPath "${PROJECT_SOURCE_DIR}/external/debug/lib/x64/assimp-vc140-mt.lib")

    add_library(assimp::assimp SHARED IMPORTED)
    set_target_properties(assimp::assimp PROPERTIES
        IMPORTED_IMPLIB         "${gvk_AssimpReleaseLIBPath}"
        IMPORTED_IMPLIB_DEBUG   "${gvk_AssimpDebugLIBPath}"
        IMPORTED_LOCATION       "${gvk_AssimpReleaseDLLPath}"
        IMPORTED_LOCATION_DEBUG "${gvk_AssimpDebugDLLPath}"
        IMPORTED_CONFIGURATIONS "RELEASE;DEBUG")
endif(UNIX)