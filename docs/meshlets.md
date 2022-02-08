# Meshlets

When using _graphics mesh pipelines_ with _task_ and _mesh shaders_, a typical use case is to segmented meshes into smaller segments called meshlets, and process these. These meshlets contain multiple triangles that should ideally be structured in a meaningful way in memory in order to enable efficient processing, like for example good memory and cache coherency. Meshlets of triangle meshes typically consist of relatively small packages of vertices and triangles of the original mesh geometry.
More information on how the meshlet pipeline works can be found on Nvidia's developer blog: [Christoph Kubisch - Introduction to Turing Mesh Shaders](https://developer.nvidia.com/blog/introduction-turing-mesh-shaders/).

## Dividing Meshes into Meshlets in Gears-Vk

Gears-Vk provides utility functions that help to divide a mesh into meshlets and convert the resulting data structure to one that can be directly used on the GPU. The implementation can be found in [meshlet_helpers.hpp](../framework/include/meshlet_helpers.hpp) and [meshlet_helpers.cpp](../framework/src/meshlet_helpers.cpp).

As a first step, the models and mesh indices that are to be divided into meshlets need to be selected. The helper function `gvk::make_selection_of_shared_models_and_mesh_indices()` can be used for this purpose.

The resulting collection can be used with one of the overloads of `gvk::divide_into_meshlets()`. If no custom division function is provided to this helper function, a simple algorithm (via `gvk::basic_meshlet_divider()`) is used by default, which just combines consecutive vertices into a meshlet until it is full w.r.t. its parameters `aMaxVertices` and `aMaxIndices`. It should be noted that this will likely not result in good vertex reuse, but is a quick way to get up and running. 

The function `gvk::divide_into_meshlets()` also offers a custom division function to be passed as parameter. This custom division function allows the usage of custom division algorithms, as provided through external libraries like [meshoptimizer](https://github.com/zeux/meshoptimizer), for example.

### Using a Custom Division Function

The custom division function (parameter `aMeshletDivision` to `gvk::divide_into_meshlets()`) can either receive the vertices and indices or just the indices depending on the use-case.
Additionally it receives the model and optionally the mesh index. If no mesh index is provided then the meshes were combined beforehand.
Ownership of the model **must not** be taken within the body of the custom `aMeshletDivision` function. 

Please note that the model will be assigned to each [`meshlet`](../framework/include/meshlet_helpers.hpp#L7) after `aMeshletDivision` has executed (see implementation of [`gvk::divide_indexed_geometry_into_meshlets()`](../framework/include/meshlet_helpers.hpp#L139).

The custom division function must follow a specific declaration schema, optional parameters can be omitted, but all of them need to be provided in the following order:

| Parameter | Mandatory? | Description |
| :------ | :---: | :--- |
| `const std::vector<glm::vec3>& tVertices` | no | The vertices of the mesh, or of combined meshes of the model |
| `const std::vector<uint32_t>& tIndices` | yes | The indices of the mesh, or of combined meshes of the model | 
| `const model_t& tModel` | yes	| The model these meshlets are generated from | 
| `std::optional<mesh_index_t> tMeshIndex` | yes | The optional mesh index. If no value is passed, it means the meshes of the model are combined into a single vertex and index buffer. | 
| `uint32_t tMaxVertices` | yes | The maximum number of vertices that should be added to a single meshlet | 
| `uint32_t tMaxIndices` | yes | The maximum number of indices that should be added to a single meshlet | 

Return value:

`std::vector<meshlet>`: The generated meshlets

#### Example for Vertices and Indices

Example of a custom division function that can be passed to `gvk::divide_into_meshlets()`, that takes both, vertices and indices:

```C++
[](const std::vector<glm::vec3>& tVertices, const std::vector<uint32_t>& aIndices,
    const gvk::model_t& aModel, std::optional<gvk::mesh_index_t> aMeshIndex,
    uint32_t aMaxVertices, uint32_t aMaxIndices) {

    std::vector<gvk::meshlet> generatedMeshlets;
    // Perform meshlet division here
    return generatedMeshlets;
}
```

#### Example for Indices Only

Example of a custom division function that can be passed to `gvk::divide_into_meshlets()`, that takes indices only:

```C++
[](const std::vector<uint32_t>& aIndices,
    const gvk::model_t& aModel, std::optional<gvk::mesh_index_t> aMeshIndex,
    uint32_t aMaxVertices, uint32_t aMaxIndices) {

    std::vector<gvk::meshlet> generatedMeshlets;
    // perform meshlet division here
    return generatedMeshlets;
}
```

#### Example That Uses a 3rd Party Library

An example about how [meshoptimizer](https://github.com/zeux/meshoptimizer) _could_ be used via the custom division function that can be passed to `gvk::divide_into_meshlets()`:

Please note: meshoptimizer expects `aMaxIndices` to be divisible by 4. Use 124 for Nvidia!

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
