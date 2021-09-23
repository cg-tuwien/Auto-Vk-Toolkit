add_executable(framebuffer
    source/framebuffer.cpp)
target_include_directories(framebuffer PRIVATE ${PROJECT_NAME})
target_link_libraries(framebuffer PRIVATE ${PROJECT_NAME})

get_target_property(framebuffer_BINARY_DIR framebuffer BINARY_DIR)

add_post_build_commands(framebuffer
    ${PROJECT_SOURCE_DIR}/examples/framebuffer/shaders
    ${framebuffer_BINARY_DIR}/shaders
    $<TARGET_FILE_DIR:framebuffer>/assets
    ""
    ${gvk_CreateDependencySymlinks})