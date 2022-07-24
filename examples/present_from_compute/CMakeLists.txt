add_executable(present_from_compute
    source/present_from_compute.cpp)
target_include_directories(present_from_compute PRIVATE ${PROJECT_NAME})
target_link_libraries(present_from_compute PRIVATE ${PROJECT_NAME})

get_target_property(present_from_compute_BINARY_DIR present_from_compute BINARY_DIR)

add_post_build_commands(present_from_compute
    ${PROJECT_SOURCE_DIR}/examples/present_from_compute/shaders
    ${present_from_compute_BINARY_DIR}/shaders
    $<TARGET_FILE_DIR:present_from_compute>/assets
    ""
    ${gvk_CreateDependencySymlinks})