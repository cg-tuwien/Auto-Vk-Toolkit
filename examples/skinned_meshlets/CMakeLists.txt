add_executable(skinned_meshlets
    source/skinned_meshlets.cpp)
target_include_directories(skinned_meshlets PRIVATE
    source)
target_include_directories(skinned_meshlets PRIVATE ${PROJECT_NAME})
target_link_libraries(skinned_meshlets PRIVATE ${PROJECT_NAME})

get_target_property(skinned_meshlets_BINARY_DIR skinned_meshlets BINARY_DIR)

set(skinned_meshlets_assets
    ${PROJECT_SOURCE_DIR}/assets/3rd_party/models/dancing_crab)

add_post_build_commands(skinned_meshlets
    ${PROJECT_SOURCE_DIR}/examples/skinned_meshlets/shaders
    ${skinned_meshlets_BINARY_DIR}/shaders
    $<TARGET_FILE_DIR:skinned_meshlets>/assets
    "${skinned_meshlets_assets}"
    ${avk_toolkit_CreateDependencySymlinks})
