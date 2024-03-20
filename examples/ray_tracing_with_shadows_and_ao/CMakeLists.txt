add_executable(ray_tracing_with_shadows_and_ao
    source/ray_tracing_with_shadows_and_ao.cpp)
target_include_directories(ray_tracing_with_shadows_and_ao PRIVATE ${PROJECT_NAME})
target_link_libraries(ray_tracing_with_shadows_and_ao PRIVATE ${PROJECT_NAME})

get_target_property(ray_tracing_with_shadows_and_ao_BINARY_DIR ray_tracing_with_shadows_and_ao BINARY_DIR)

set(ray_tracing_with_shadows_and_ao_assets
    ${PROJECT_SOURCE_DIR}/assets)

add_post_build_commands(ray_tracing_with_shadows_and_ao
    ${PROJECT_SOURCE_DIR}/examples/ray_tracing_with_shadows_and_ao/shaders
    ${ray_tracing_with_shadows_and_ao_BINARY_DIR}/shaders
    ${ray_tracing_with_shadows_and_ao_BINARY_DIR}/assets
    # $<TARGET_FILE_DIR:ray_tracing_with_shadows_and_ao>/assets
    "${ray_tracing_with_shadows_and_ao_assets}"
    ${avk_toolkit_CreateDependencySymlinks})