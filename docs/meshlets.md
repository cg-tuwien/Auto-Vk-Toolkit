# Meshlets

When using mesh shaders, the mesh is segmented into smaller segments called meshlets. These meshlets contain multiple triangles that should ideally be directly attached to one another to leverage the better vertex reuse capabilities of the mesh shader pipeline. 

More information on how the meshlet pipeline works can be found on [Nvidias developer blog.](https://developer.nvidia.com/blog/introduction-turing-mesh-shaders/)

## Meshlets in gears-vk

Gears-vk provides some methods that help you divide your mesh into meshlets and convert the resulting data structure to one that can be directly used on the GPU.

First you need to select which meshes and model indices you want to divide into meshlet. You can either make your own collection or use the helper function `gvk::make_selection_of_shared_models_and_mesh_indices()` . 

The resulting collection can then be used with the `gvk::divide_into_meshlets()` functions. If you do not provide your own division function to this helper function, a simple algorithm (`gvk::basic_meshlet_divider()`) is used that just puts consecutive vertices into a meshlet until it is full. Though it should be noted that this will likely not result in good vertex reuse, but is a quick way to get up and running. 

The `gvk::divide_into_meshlets()` also allows you to provide your own division function. This provides you with the ability to use your own division algorithms or use an external library like [meshoptimizer](https://github.com/zeux/meshoptimizer) for example.

But those meshlets cannot be directly used on the GPU as they contain additional information and are int non suitable containers. Therefore you need to convert these into a suitable format for the GPU. For this you can use the `gvk::convert_for_gpu_usage<T>()` utility functions. 

There are two GPU suitable types already defined that work with this utility functions. One is `gvk::meshlet_gpu_data` and the other `gvk::meshlet_redirected_gpu_data`. The difference between those is that `gvk::meshlet_gpu_data` has the vertex indices of the meshlet directly stored in the meshlet and `gvk::meshlet_redirected_gpu_data` uses a separate vertex index array that is indexed by the meshlet. Therefore the redirected version reduces the memory footprint of a meshlet while introducing an additional indirection into the separate index buffer.

If you want to convert the meshlets into your own GPU suitable format, have a look at the implementation and make your own or - if applicable - convert it first into the gears-vk format and then convert it to yours.

### Using a custom division function

The callback can either receive the vertices and indices or just the indices depending on your specific needs.
Additionally it provides the model and an optional mesh index if more data is needed. If no mesh index is provided then the meshes were combined beforehand.
Ownership of the model must not be taken within the body of aMeshletDivision. The model will be assigned to each `meshlet` after aMeshletDivision has executed.

The callback must follow a specific declaration schema, optional parameters can be omitted, but all of them need to be provided in the following order:

| Parameter | Mandatory ? | Description |
| --- | --- | --- |
| `const std::vector<glm::vec3>& tVertices` | no | The vertices of the mesh or combined meshes of the model. |
| `const std::vector<uint32_t>& tIndices` | yes | The indices of the mesh or combined meshes of the model. | 
| `const model_t& tModel` | yes	| The model these meshlets are generated from. | 
| `std::optional<mesh_index_t> tMeshIndex` | yes | The optional mesh index. If no value is provided, it means the meshes of the model are combined into a single vertex and index buffer. | 
| `uint32_t tMaxVertices` | yes | The maximum number of vertices that are allowed in a single meshlet. | 
| `uint32_t tMaxIndices` | yes | The maximum number of indices that are allowed in a single meshlet. | 

Return value:

`std::vector<meshlet>`: The generated meshlets

A basic example: 

```C++
[](const std::vector<glm::vec3>& tVertices, const std::vector<uint32_t>& aIndices,
    const model_t& aModel, std::optional<mesh_index_t> aMeshIndex,
    uint32_t aMaxVertices, uint32_t aMaxIndices) {

    std::vector<gvk::meshlet> generatedMeshlets;
    // Do your meshlet division here
    return generatedMeshlets;
}
```