# Meshlets

When using mesh shaders, the mesh is segmented into smaller segments called meshlets. These meshlets contain multiple triangles that should ideally be directly attached to one another to leverage the better vertex reuse capabilities of the mesh shader pipeline. 

More information on how the meshlet pipeline works can be found on [Nvidias developer blog.](https://developer.nvidia.com/blog/introduction-turing-mesh-shaders/)

## Meshlets in Gears-Vk

Gears-Vk provides some methods that help divide a mesh into meshlets and convert the resulting data structure to one that can be directly used on the GPU. The implementation can be found in [meshlet_helpers.hpp](../framework/include/meshlet_helpers.hpp) and [meshlet_helpers.cpp](../framework/src/meshlet_helpers.cpp).

First the meshes and models indices that are to be divided into meshlets need to be selected. The selection can be created from scratch or with the helper function `gvk::make_selection_of_shared_models_and_mesh_indices()`.

The resulting collection can then be used with the `gvk::divide_into_meshlets()` functions. If no division function is provided to this helper function, a simple algorithm (`gvk::basic_meshlet_divider()`) is used that just puts consecutive vertices into a meshlet until it is full. Though it should be noted that this will likely not result in good vertex reuse, but is a quick way to get up and running. 

The `gvk::divide_into_meshlets()` also offers to provide a custom division function. This allows the usage of custom division algorithms or the use of external libraris like [meshoptimizer](https://github.com/zeux/meshoptimizer) for example.

### Using a custom division function

The callback can either receive the vertices and indices or just the indices depending on the use-case.
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
    const gvk::model_t& aModel, std::optional<gvk::mesh_index_t> aMeshIndex,
    uint32_t aMaxVertices, uint32_t aMaxIndices) {

    std::vector<gvk::meshlet> generatedMeshlets;
    // Perform meshlet division here
    return generatedMeshlets;
}
```

An example just with indices and no vertices:

```C++
[](const std::vector<uint32_t>& aIndices,
    const gvk::model_t& aModel, std::optional<gvk::mesh_index_t> aMeshIndex,
    uint32_t aMaxVertices, uint32_t aMaxIndices) {

    std::vector<gvk::meshlet> generatedMeshlets;
    // perform meshlet division here
    return generatedMeshlets;
}
```


An example on how [meshoptimizer](https://github.com/zeux/meshoptimizer) _could_ be used:

(meshoptimizer expects the aMaxIndices to be divisible by 4, so use 124 for Nvidia as an example)
```C++
[](const std::vector<glm::vec3>& tVertices, const std::vector<uint32_t>& aIndices,
    const gvk::model_t& aModel, std::optional<gvk::mesh_index_t> aMeshIndex,
    uint32_t aMaxVertices, uint32_t aMaxIndices) {

    // definitions
    size_t max_triangles = aMaxIndices/3;
    const float cone_weight = 0.0f;

    // get the maximum number of meshlets that could be generated
    size_t max_meshlets = meshopt_buildMeshletsBound(aIndices.size(), aMaxVertices, max_triangles);
    std::vector<meshopt_Meshlet> meshlets(max_meshlets);
    std::vector<unsigned int> meshlet_vertices(max_meshlets * aMaxVertices);
    std::vector<unsigned char> meshlet_triangles(max_meshlets * max_triangles * 3);

    // let meshoptimizer build the meshlets for us
    size_t meshlet_count = meshopt_buildMeshlets(meshlets.data(), meshlet_vertices.data(), meshlet_triangles.data(), 
                                                aIndices.data(), aIndices.size(), &tVertices[0].x, tVertices.size(), sizeof(glm::vec3), 
                                                aMaxVertices, max_triangles, cone_weight);

    // copy the data over to the gears-vk meshlet structure
    std::vector<gvk::meshlet> generatedMeshlets(meshlet_count);
    generatedMeshlets.resize(meshlet_count);
    generatedMeshlets.reserve(meshlet_count);
    for(int k = 0; k < meshlet_count; k++) {
        auto& m = meshlets[k];
        auto& gm = generatedMeshlets[k];
        gm.mIndexCount = m.triangle_count * 3;
        gm.mVertexCount = m.vertex_count;
        gm.mVertices.reserve(m.vertex_count);
        gm.mVertices.resize(m.vertex_count);
        gm.mIndices.reserve(gm.mIndexCount);
        gm.mIndices.resize(gm.mIndexCount);
        std::ranges::copy(meshlet_vertices.begin() + m.vertex_offset,
                        meshlet_vertices.begin() + m.vertex_offset + m.vertex_count,
                        gm.mVertices.begin());
        std::ranges::copy(meshlet_triangles.begin() + m.triangle_offset,
                        meshlet_triangles.begin() + m.triangle_offset + gm.mIndexCount,
                        gm.mIndices.begin());
    }

    return generatedMeshlets;
}
```

## Converting into a format for GPU usage

Meshlets in such a format cannot be directly used on the GPU as they contain additional information which is aslo in non suitable containers. Therefore these need to be converted into a suitable format for the GPU. For this the `gvk::convert_for_gpu_usage<T>()` utility functions can be used. 

```c++
/** Meshlet for GPU usage
    *  @tparam NV	The number of vertices
    *	@tparam NI	The number of indices
    */
template <size_t NV = 64, size_t NI = 378>
struct meshlet_gpu_data
{
    static const size_t sNumVertices = NV;
    static const size_t sNumIndices = NI;

    /** Vertex indices into the vertex array */
    uint32_t mVertices[NV];
    /** Indices into the vertex indices */
    uint8_t mIndices[NI];  
    /** The vertex count */
    uint8_t mVertexCount;
    /** The primitive count */
    uint8_t mPrimitiveCount;
};

/** Meshlet for GPU usage in combination with the meshlet data generated by convert_for_gpu_usage */
struct meshlet_redirected_gpu_data
{
    /** Data offset into the meshlet data array */
    uint32_t mDataOffset;
    /** The vertex count */
    uint8_t mVertexCount;
    /** The primitive count */
    uint8_t mPrimitiveCount;
};
```

There are two GPU suitable types already defined that work with this utility functions. One is `gvk::meshlet_gpu_data` and the other `gvk::meshlet_redirected_gpu_data`. The difference between those is that `gvk::meshlet_gpu_data` has the vertex indices of the meshlet directly stored in the meshlet and `gvk::meshlet_redirected_gpu_data` uses a separate vertex index array that is indexed by the meshlet. Therefore the redirected version reduces the memory footprint of a meshlet while introducing an additional indirection into the separate index buffer.

If a custom GPU suitable format is needed, our implementation can be used as a starting point. Otherwise gears-vr can also be used to convert it into it's GPU format first that can then be converted into a different format if so desired.
