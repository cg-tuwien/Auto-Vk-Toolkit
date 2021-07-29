cmake_minimum_required(VERSION 3.14)

include(FetchContent)

# TODO: add option to use local build of assimp
#  this would need to set assimp_SOURCE_DIR (e.g. to external/universal)
#  and to set assimp::assimp to the .so/.dll

set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
set(ASSIMP_BUILD_TESTS OFF)
set(INJECT_DEBUG_POSTFIX OFF)

FetchContent_Declare(
        assimp
        GIT_REPOSITORY      https://github.com/assimp/assimp.git
        GIT_TAG             v5.0.1
)

FetchContent_MakeAvailable(assimp)
