cmake_minimum_required(VERSION 3.14)

include(FetchContent)

FetchContent_Declare(
        simplefilewatcher
        GIT_REPOSITORY      https://github.com/jameswynn/simplefilewatcher.git
        GIT_TAG             master
)

FetchContent_MakeAvailable(simplefilewatcher)

add_library(simplefilewatcher INTERFACE)
target_include_directories(simplefilewatcher INTERFACE
        ${simplefilewatcher_SOURCE_DIR}/include)
target_sources(simplefilewatcher INTERFACE
        ${simplefilewatcher_SOURCE_DIR}/source/FileWatcher.cpp
        ${simplefilewatcher_SOURCE_DIR}/source/FileWatcherLinux.cpp
        ${simplefilewatcher_SOURCE_DIR}/source/FileWatcherOSX.cpp
        ${simplefilewatcher_SOURCE_DIR}/source/FileWatcherWin32.cpp)
