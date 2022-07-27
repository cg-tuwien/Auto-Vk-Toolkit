add_executable(hello_world
    source/hello_world.cpp)
target_include_directories(hello_world PRIVATE ${PROJECT_NAME})
target_link_libraries(hello_world PRIVATE ${PROJECT_NAME})

get_target_property(hello_world_BINARY_DIR hello_world BINARY_DIR)

add_post_build_commands(hello_world
    ${PROJECT_SOURCE_DIR}/examples/hello_world/shaders
    ${hello_world_BINARY_DIR}/shaders
    $<TARGET_FILE_DIR:hello_world>/assets
    ""
    ${avk_toolkit_CreateDependencySymlinks})