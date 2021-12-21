add_executable(static_meshlets
    source/static_meshlets.cpp)
target_include_directories(static_meshlets PRIVATE
    source)
target_include_directories(static_meshlets PRIVATE ${PROJECT_NAME})
target_link_libraries(static_meshlets PRIVATE ${PROJECT_NAME})

get_target_property(static_meshlets_BINARY_DIR static_meshlets BINARY_DIR)

set(static_meshlets_assets
    ${PROJECT_SOURCE_DIR}/assets/3rd_party/models/stanford_bunny)

add_post_build_commands(static_meshlets
    ${PROJECT_SOURCE_DIR}/examples/static_meshlets/shaders
    ${static_meshlets_BINARY_DIR}/shaders
    $<TARGET_FILE_DIR:static_meshlets>/assets
    "${static_meshlets_assets}"
    ${gvk_CreateDependencySymlinks})