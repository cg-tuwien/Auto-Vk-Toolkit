add_executable(vertex_buffers
    source/vertex_buffers.cpp)
target_include_directories(vertex_buffers PRIVATE ${PROJECT_NAME})
target_link_libraries(vertex_buffers PRIVATE ${PROJECT_NAME})

get_target_property(vertex_buffers_BINARY_DIR vertex_buffers BINARY_DIR)

add_post_build_commands(vertex_buffers
    ${PROJECT_SOURCE_DIR}/examples/vertex_buffers/shaders
    ${vertex_buffers_BINARY_DIR}/shaders
    ${vertex_buffers_BINARY_DIR}/assets
    # $<TARGET_FILE_DIR:vertex_buffers>/assets
    ""
    ${avk_toolkit_CreateDependencySymlinks})