cmake_minimum_required(VERSION 3.14)

include(FetchContent)

# Move this to top level, because it affects assimp & glfw
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "Build static libraries")

#option(BUILD_SHARED_LIBS "Build package with shared libraries." OFF)

FetchContent_Declare(
        assimp
        GIT_REPOSITORY      https://github.com/assimp/assimp.git
        GIT_TAG             v5.0.1
)

FetchContent_MakeAvailable(assimp)
