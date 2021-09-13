add_executable(texture_cubemap
    source/texture_cubemap.cpp)
target_include_directories(texture_cubemap PRIVATE ${PROJECT_NAME})
target_link_libraries(texture_cubemap PRIVATE ${PROJECT_NAME})

get_target_property(texture_cubemap_BINARY_DIR texture_cubemap BINARY_DIR)

set(texture_cubemap_assets
    ${PROJECT_SOURCE_DIR}/assets/3rd_party/textures/yokohama_at_night
    ${PROJECT_SOURCE_DIR}/assets/1st_party/models
    ${PROJECT_SOURCE_DIR}/assets/3rd_party/models/standford_bunny)

add_post_build_commands(texture_cubemap
    ${PROJECT_SOURCE_DIR}/examples/texture_cubemap/shaders
    ${texture_cubemap_BINARY_DIR}/shaders
    $<TARGET_FILE_DIR:texture_cubemap>/assets
    "${texture_cubemap_assets}"
    ${gvk_CreateDependencySymlinks})