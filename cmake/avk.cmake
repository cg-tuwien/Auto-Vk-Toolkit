cmake_minimum_required(VERSION 3.14)

include(FetchContent)

FetchContent_Declare(
    avk
    GIT_REPOSITORY      https://github.com/cg-tuwien/Auto-Vk.git
    GIT_TAG             156d943fb1c19af1c986ea77b578e895bf967c02 #master
)

FetchContent_MakeAvailable(avk)
