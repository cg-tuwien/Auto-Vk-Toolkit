cmake_minimum_required(VERSION 3.14)

include(FetchContent)

FetchContent_Declare(
        assimp
        GIT_REPOSITORY      https://github.com/assimp/assimp.git
        GIT_TAG             v5.0.1
)

FetchContent_MakeAvailable(assimp)
