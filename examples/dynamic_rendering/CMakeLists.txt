add_executable(dynamic_rendering
    source/dynamic_rendering.cpp)
target_include_directories(dynamic_rendering PRIVATE ${PROJECT_NAME})
target_link_libraries(dynamic_rendering PRIVATE ${PROJECT_NAME})

get_target_property(dynamic_rendering_BINARY_DIR dynamic_rendering BINARY_DIR)

add_post_build_commands(dynamic_rendering
    ${PROJECT_SOURCE_DIR}/examples/dynamic_rendering/shaders
    ${dynamic_rendering_BINARY_DIR}/shaders
    $<TARGET_FILE_DIR:dynamic_rendering>/assets
    ""
    ${avk_toolkit_CreateDependencySymlinks})