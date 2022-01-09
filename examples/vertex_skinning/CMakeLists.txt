add_executable(
        vertex_skinning
        source/vertex_skinning.cpp
        source/animated_model.cpp
        source/animated_model.hpp
        source/dual_quaternion.cpp
        source/dual_quaternion.hpp
        source/quaternion.cpp
        source/quaternion.hpp
)
target_include_directories(vertex_skinning PRIVATE ${PROJECT_NAME})
target_link_libraries(vertex_skinning PRIVATE ${PROJECT_NAME})

get_target_property(vertex_skinning_BINARY_DIR vertex_skinning BINARY_DIR)

set(vertex_skinning_assets
    ${PROJECT_SOURCE_DIR}/assets/1st_party/models/skinning)

add_post_build_commands(vertex_skinning
    ${PROJECT_SOURCE_DIR}/examples/vertex_skinning/shaders
    ${vertex_skinning_BINARY_DIR}/shaders
    $<TARGET_FILE_DIR:vertex_skinning>/assets
    "${vertex_skinning_assets}"
    ${gvk_CreateDependencySymlinks})