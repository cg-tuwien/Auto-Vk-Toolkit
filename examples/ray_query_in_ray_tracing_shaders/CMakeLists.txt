add_executable(ray_query_in_ray_tracing_shaders
    source/ray_query_in_ray_tracing_shaders.cpp)
target_include_directories(ray_query_in_ray_tracing_shaders PRIVATE
    source)
target_include_directories(ray_query_in_ray_tracing_shaders PRIVATE ${PROJECT_NAME})
target_link_libraries(ray_query_in_ray_tracing_shaders PRIVATE ${PROJECT_NAME})

get_target_property(ray_query_in_ray_tracing_shaders_BINARY_DIR ray_query_in_ray_tracing_shaders BINARY_DIR)

set(ray_query_in_ray_tracing_shaders_assets
    ${PROJECT_SOURCE_DIR}/assets)

add_post_build_commands(ray_query_in_ray_tracing_shaders
    ${PROJECT_SOURCE_DIR}/examples/ray_query_in_ray_tracing_shaders/shaders
    ${ray_query_in_ray_tracing_shaders_BINARY_DIR}/shaders
    $<TARGET_FILE_DIR:ray_query_in_ray_tracing_shaders>/assets
    "${ray_query_in_ray_tracing_shaders_assets}"
    ${avk_toolkit_CreateDependencySymlinks})