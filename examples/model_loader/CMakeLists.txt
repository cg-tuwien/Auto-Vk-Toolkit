add_executable(model_loader
    source/model_loader.cpp)
target_include_directories(model_loader PRIVATE
    source)
target_include_directories(model_loader PRIVATE ${PROJECT_NAME})
target_link_libraries(model_loader PRIVATE ${PROJECT_NAME})

get_target_property(model_loader_BINARY_DIR model_loader BINARY_DIR)

set(model_loader_assets
    ${PROJECT_SOURCE_DIR}/assets/3rd_party/models/sponza)

add_post_build_commands(model_loader
    ${PROJECT_SOURCE_DIR}/examples/model_loader/shaders
    ${model_loader_BINARY_DIR}/shaders
    $<TARGET_FILE_DIR:model_loader>/assets
    "${model_loader_assets}"
    ${avk_toolkit_CreateDependencySymlinks})
