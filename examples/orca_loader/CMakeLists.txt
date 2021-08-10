add_executable(orca_loader
    source/orca_loader.cpp)
target_include_directories(orca_loader PRIVATE
    source)
target_include_directories(orca_loader PRIVATE ${PROJECT_NAME})
target_link_libraries(orca_loader PRIVATE ${PROJECT_NAME})

get_target_property(orca_loader_BINARY_DIR orca_loader BINARY_DIR)

set(orca_loader_assets
    ${PROJECT_SOURCE_DIR}/assets)

add_post_build_commands(orca_loader
    ${PROJECT_SOURCE_DIR}/examples/orca_loader/shaders
    ${orca_loader_BINARY_DIR}/shaders
    $<TARGET_FILE_DIR:orca_loader>/assets
    "${orca_loader_assets}"
    ${gvk_CreateDependencySymlinks})