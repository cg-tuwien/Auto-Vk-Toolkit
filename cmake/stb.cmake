cmake_minimum_required(VERSION 3.11)

include(FetchContent)

FetchContent_Declare(
        stb
        GIT_REPOSITORY      https://github.com/nothings/stb.git
        GIT_TAG             master
)

FetchContent_GetProperties(stb)
if(NOT stb_POPULATED)
    FetchContent_Populate(stb)
endif()
