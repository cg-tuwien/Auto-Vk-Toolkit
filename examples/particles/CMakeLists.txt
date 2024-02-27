add_executable(particles
    source/hello_world.cpp)
target_include_directories(particles PRIVATE ${PROJECT_NAME})
target_link_libraries(particles PRIVATE ${PROJECT_NAME})

get_target_property(particles_BINARY_DIR particles BINARY_DIR)

add_post_build_commands(particles
    ${PROJECT_SOURCE_DIR}/examples/particles/shaders
    ${particles_BINARY_DIR}/shaders
    $<TARGET_FILE_DIR:particles>/assets
    ""
    ${avk_toolkit_CreateDependencySymlinks})