add_executable(multi_invokee_rendering
    source/multi_invokee_rendering.cpp)
target_include_directories(multi_invokee_rendering PRIVATE ${PROJECT_NAME})
target_link_libraries(multi_invokee_rendering PRIVATE ${PROJECT_NAME})

get_target_property(multi_invokee_rendering_BINARY_DIR multi_invokee_rendering BINARY_DIR)

add_post_build_commands(multi_invokee_rendering
    ${PROJECT_SOURCE_DIR}/examples/multi_invokee_rendering/shaders
    ${multi_invokee_rendering_BINARY_DIR}/shaders
    ${multi_invokee_rendering_BINARY_DIR}/assets
    # $<TARGET_FILE_DIR:multi_invokee_rendering>/assets
    ""
    ${avk_toolkit_CreateDependencySymlinks})