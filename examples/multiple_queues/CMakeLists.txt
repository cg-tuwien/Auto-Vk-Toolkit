add_executable(multiple_queues
    source/multiple_queues.cpp)
target_include_directories(multiple_queues PRIVATE ${PROJECT_NAME})
target_link_libraries(multiple_queues PRIVATE ${PROJECT_NAME})

get_target_property(multiple_queues_BINARY_DIR multiple_queues BINARY_DIR)

add_post_build_commands(multiple_queues
    ${PROJECT_SOURCE_DIR}/examples/multiple_queues/shaders
    ${multiple_queues_BINARY_DIR}/shaders
    $<TARGET_FILE_DIR:multiple_queues>/assets
    ""
    ${gvk_CreateDependencySymlinks})