add_executable(ray_tracing_custom_intersection
    source/ray_tracing_custom_intersection.cpp)
target_include_directories(ray_tracing_custom_intersection PRIVATE
    source)
target_include_directories(ray_tracing_custom_intersection PRIVATE ${PROJECT_NAME})
target_link_libraries(ray_tracing_custom_intersection PRIVATE ${PROJECT_NAME})

get_target_property(ray_tracing_custom_intersection_BINARY_DIR ray_tracing_custom_intersection BINARY_DIR)

add_post_build_commands(ray_tracing_custom_intersection
    ${PROJECT_SOURCE_DIR}/examples/ray_tracing_custom_intersection/shaders
    ${ray_tracing_custom_intersection_BINARY_DIR}/shaders
    ${ray_tracing_custom_intersection_BINARY_DIR}/assets
    # $<TARGET_FILE_DIR:ray_tracing_custom_intersection>/assets
    ""
    ${avk_toolkit_CreateDependencySymlinks})