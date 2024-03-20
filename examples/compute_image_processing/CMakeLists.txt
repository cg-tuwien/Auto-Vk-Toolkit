add_executable(compute_image_processing
   source/compute_image_processing.cpp)
target_include_directories(compute_image_processing PRIVATE ${PROJECT_NAME})
target_link_libraries(compute_image_processing PRIVATE ${PROJECT_NAME})

get_target_property(compute_image_processing_BINARY_DIR compute_image_processing BINARY_DIR)

set(compute_image_processing_assets
    ${PROJECT_SOURCE_DIR}/assets/3rd_party/models/sponza/textures)

add_post_build_commands(compute_image_processing
    ${PROJECT_SOURCE_DIR}/examples/compute_image_processing/shaders
    ${compute_image_processing_BINARY_DIR}/shaders
    ${compute_image_processing_BINARY_DIR}/assets
    # $<TARGET_FILE_DIR:compute_image_processing>/assets
    "${compute_image_processing_assets}"
    ${avk_toolkit_CreateDependencySymlinks})