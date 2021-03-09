# Serializer
_Gears-Vk_ features an object serializer to improve load times of resources like huge 3D models or ORCA scenes by serializing all the processed and ready to use data into a cache file during the first run. In further runs, the data can be directly deserialized into types and structures used by _Gears-Vk_ and expensive parsing and processing of huge models or scenes can be avoided.

# How to use
Serialization functionality is available via `gvk::serializer` class. The serializer is initialized
by providing a path to the cache file and the desired mode. If the mode is set to `gvk::serializer::mode::serialize`, the file at the given path is created or overwritten if it already exists. If the mode is set to `gvk::serializer::mode::deserialize`, the serializer reads from the cache file at the given path. A helper function `gvk::does_cache_file_exist(const std::string_view)` can be used to determine which initialization mode shall be used. The serializer initialization for storing or loading a cached ORCA scene could look like follows:
```
const std::string cacheFilePath(pathToOrcaScene + ".cache");

auto serializer = gvk::serializer(cacheFilePath, gvk::does_cache_file_exist(cacheFilePath) ? gvk::serializer::mode::deserialize : gvk::serializer::mode::serialize);
```
A convenience constructor is available, if the serializer shall determine the mode to use. If the file at the given path exists, the serializer is initialized for reading in mode `gvk::serializer::mode::deserialize`, else for writing in mode `gvk::serializer::mode::deserialize`:
```
auto serializer = gvk::serializer(cacheFilePath);
```
After initialization, the current _mode_ of the serializer can be retrived by calling `mode()` on the serializer, which either returns `gvk::serializer::mode::serialize` or `gvk::serializer::mode::deserialize`. To serialize or deserialize a type, the `gvk::serializer::archive(Type&&)` function template is called on the serializer object. This function serializes the given object to the cache file if the mode is `gvk::serializer::mode::serialize`, or deserializes the object from the cache file into the given object if the mode is `gvk::serializer::mode::deserialize`. Using the serializer for normals, vertices and indices can be implemented like follows:
```
std::vector<glm::vec3> normals;
std::tuple<std::vector<glm::vec3>, std::vector<uint32_t>> verticesAndIndices;
if (serializer.mode() == gvk::serializer::mode::serialize) {
	normals = gvk::get_normals(...);
	verticesAndIndices = gvk::get_vertices_and_indices(...);
}
serializer.archive(normals);
serializer.archive(verticesAndIndices);
```
First, variables are defined to hold the data. If the serializer's mode is `gvk::serializer::mode::serialize`, the vector of normals and the tuple of vertices and indices are retrieved with the helper functions in [`material_image_helpers.hpp`](https://github.com/cg-tuwien/Gears-Vk/blob/master/framework/include/material_image_helpers.hpp). `serializer.archive(...)` serializes `std::vector<glm::vec3>` and `std::tuple<std::vector<glm::vec3>, std::vector<uint32_t>` to the cache file. If the serializer's mode is `gvk::serializer::mode::deserialize`, calls to the helper functions are not necessary anymore because both variables can be retrieved from the cache file in `serializer.archive(...)`.

The serializer features specialized functions for serializing and deserializing blocks of memory and host visible buffers. Both can be used separately but are especially usefull when used in conjunction, e.g. to serialize/deserialize image data. Image data loaded from a file can be serialized using `gvk::serializer::archive_memory` and in further program runs, i.e. when deserializing, directly copied into a host visible buffer using `gvk::serializer::archive_buffer`:
```
size_t imageSize;
void* pixels;
if (serializer.mode() == gvk::serializer::mode::serialize) {
	// Load image frome file using stb image lib
	int channelsInFile = 0;
	int width = 0;
	int height = 0;
	int desiredColorChannels = STBI_rgb_alpha;
	pixels = stbi_load(pathToImage.c_str(), &width, &height, &channelsInFile, desiredColorChannels);
	imageSize = static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(desiredColorChannels);

	serializer.archive_memory(pixels, imageSize);
}
serializer.archive(imageSize);

// Create host visible staging buffer
auto sb = avk::create_buffer(
	AVK_STAGING_BUFFER_MEMORY_USAGE,
	vk::BufferUsageFlagBits::eTransferSrc,
	avk::generic_buffer_meta::create_from_size(imageSize)
);

if (serializer.mode() == gvk::serializer::mode::serialize) {
	sb->fill(pixels, 0, avk::sync::not_required());
}
else {
	serializer.archive_buffer(sb);
}
```
In the example code above image data is loaded from file via `stbi_load`, but only when the serializer's mode is `gvk::serializer::mode::serialize`. It returns a pointer to the data and the values to calculate the total image size. `serializer.archive_memory` is then used to serialize the image data to the cache file and `serializer.archive` is used to either serialize or deserialize the size of the image. After creating a host visible staging buffer with the size of the image, the buffer is either filled by its own `avk::buffer_t::fill` function or directly from the cache file using `gvk::serializer::archive_buffer`, which avoids an extra memory allocation in main memory for the image data and copies directly into a host visible GPU buffer.

## \*\_cached functions
_Gears-Vk_ features `*_cached` function variants for various work loads that support serialization/deserialization. They are intended to simplify serializer usage and avoid the need of implementing different code paths for both modes of the serializer. For example, to retrieve 2D texture coordinates buffer for model data (modelAndMeshes), `gvk::create_2d_texture_coordinates_buffer_cached` can be used:
```
auto texCoordsBuffer = gvk::create_2d_texture_coordinates_buffer_cached(
	serializer, modelAndMeshes, 0, avk::sync::wait_idle()
);
```
The equivalent functionality without usage of a `*_cached` function variant would be like follows in this case:
```
avk::buffer texCoordsBuffer;
if (serializer.mode() == gvk::serializer::mode::serialize) {
	texCoordsBuffer = gvk::create_2d_texture_coordinates_buffer(
		modelAndMeshes, 0, avk::sync::wait_idle()
	);
}
serializer.archive_buffer(texCoordsBuffer);
```
The `*_cached` functions internally either serialize or deserialize the necessary data (in the above example, that would be the content of the returned buffer, i.e. the 2D texture coordinates). All `*_cached` functions are designed to integrate well into existing, non-serialized code paths. The following code shows a non-serialized way of loading an ORCA scene and creating buffers for positions, indices and texture coordinates. The code omits iterating through all materials and just creates buffers for the first mesh that is assigned to the first material returned by `gvk::orca_scene_t::distinct_material_configs_for_all_models`:
```
// Load an ORCA scene from file
auto orca = gvk::orca_scene_t::load_from_file(aPathToOrcaScene);
// Get all the different materials from the whole scene
auto distinctMaterialsOrca = orca->distinct_material_configs_for_all_models();

// Get mesh indices of the first material
std::vector<gvk::model_and_mesh_indices> meshIndices = distinctMaterialsOrca.begin()->second;
int meshIndicesIndex = 0;

std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<size_t>>> modelAndMeshes;
modelAndMeshes = gvk::make_models_and_meshes_selection(orca->model_at_index(meshIndices[meshIndicesIndex].mModelIndex).mLoadedModel, meshIndices[meshIndicesIndex].mMeshIndices);

// Get a buffer containing all positions, and one containing all indices for all submeshes with this material
auto [positionsBuffer, indicesBuffer] = gvk::create_vertex_and_index_buffers(
	modelAndMeshes, {}, avk::sync::wait_idle()
);

// Get a buffer containing all texture coordinates for all submeshes with this material
auto texCoordsBuffer = gvk::create_2d_texture_coordinates_flipped_buffer(
	modelAndMeshes, 0, avk::sync::wait_idle()
);
```
Integrating the serializer into above code is easily done by introducing a condition at the beginning, to avoid scene loading from file, and swap some helper functions for their `*_cached` counterparts:
```
std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<size_t>>> modelAndMeshes;

if (serializer.mode() == gvk::serializer::mode::serialize) {
	// Load an ORCA scene from file
	auto orca = gvk::orca_scene_t::load_from_file(aPathToOrcaScene);
	// Get all the different materials from the whole scene
	auto distinctMaterialsOrca = orca->distinct_material_configs_for_all_models();

	// Get mesh indices of the first material
	auto meshIndices = distinctMaterialsOrca.begin()->second;
	int meshIndicesIndex = 0;

	modelAndMeshes = gvk::make_models_and_meshes_selection(orca->model_at_index(meshIndices[meshIndicesIndex].mModelIndex).mLoadedModel, meshIndices[meshIndicesIndex].mMeshIndices);
}

// Get a buffer containing all positions, and one containing all indices for all submeshes with this material
auto [positionsBuffer, indicesBuffer] = gvk::create_vertex_and_index_buffers_cached(
	serializer, modelAndMeshes, {}, avk::sync::wait_idle()
);

// Get a buffer containing all texture coordinates for all submeshes with this material
auto texCoordsBuffer = gvk::create_2d_texture_coordinates_flipped_buffer_cached(
	serializer, modelAndMeshes, 0, avk::sync::wait_idle()
);
```

For further `*_cached` function usage see `void load_orca_scene_cached(...)` in the **orca_loader** example at [`orca_loader.cpp#L187`](https://github.com/cg-tuwien/Gears-Vk/blob/master/examples/orca_loader/source/orca_loader.cpp#L187)

#### Available \*\_cached variants of scene and model loading functions
* `get_vertices_and_indices_cached(gvk::serializer& aSerializer, ...)`
* `create_vertex_and_index_buffers_cached(gvk::serializer& aSerializer, ...)`
* `get_normals_cached(gvk::serializer& aSerializer, ...)`
* `create_normals_buffer_cached(gvk::serializer& aSerializer, ...)`
* `get_tangents_cached(gvk::serializer& aSerializer, ...)`
* `create_tangents_buffer_cached(gvk::serializer& aSerializer, ...)`
* `get_bitangents_cached(gvk::serializer& aSerializer, ...)`
* `create_bitangents_buffer_cached(gvk::serializer& aSerializer, ...)`
* `get_colors_cached(gvk::serializer& aSerializer, ...)`
* `create_colors_buffer_cached(gvk::serializer& aSerializer, ...)`
* `get_bone_weights_cached(gvk::serializer& aSerializer, ...)`
* `create_bone_weights_buffer_cached(gvk::serializer& aSerializer, ...)`
* `create_bone_weights_buffer_cached(gvk::serializer& aSerializer, ...)`
* `get_bone_indices_cached(gvk::serializer& aSerializer, ...)`
* `create_bone_indices_buffer_cached(gvk::serializer& aSerializer, ...)`
* `get_bone_indices_for_single_target_buffer_cached(gvk::serializer& aSerializer, ...)`
* `create_bone_indices_for_single_target_buffer_buffer_cached(gvk::serializer& aSerializer, ...)`
* `get_bone_indices_for_single_target_buffer_cached(gvk::serializer& aSerializer, ...)`
* `create_bone_indices_for_single_target_buffer_buffer_cached(gvk::serializer& aSerializer, ...)`
* `get_2d_texture_coordinates_cached(gvk::serializer& aSerializer, ...)`
* `create_2d_texture_coordinates_buffer_cached(gvk::serializer& aSerializer, ...)`
* `get_2d_texture_coordinates_flipped_cached(gvk::serializer& aSerializer, ...)`
* `create_2d_texture_coordinates_flipped_buffer_cached(gvk::serializer& aSerializer, ...)`
* `get_3d_texture_coordinates_cached(gvk::serializer& aSerializer, ...)`
* `create_3d_texture_coordinates_buffer_cached(gvk::serializer& aSerializer, ...)`
* `convert_for_gpu_usage_cached(gvk::serializer& aSerializer, ...)`


## Custom type serialization
To serialize a custom type, the custom type is required to have a specialized overload to the `serialize` template function which defines how a type is serialized. For commonly used types such as `glm::vec3`, `glm::mat4`, etc. and various types in the `std` namespace such `serialize` overloads are already defined in [`serializer.hpp`](https://github.com/cg-tuwien/Gears-Vk/blob/master/framework/include/serializer.hpp).

To add serialization ability to a custom type, a `serialize` function template overload in the same namespace as the custom type must be created. The function template takes two arguments, an `Archive` and the custom type's type `Type`. Both parameters must be passed by reference. If one of the members is yet another custom type, a separate `gvk::serialize` function template overload must be created.
```
nampespace SOME_NAMESPACE {
	struct SOME_TYPE {
		member1;
		member2;
		...
	};

	template<typename Archive>
	void serialize(Archive& aArchive, SOME_TYPE& aValue)
	{
		aArchive(aValue.member1, aValue.member2, ...);
	}
}
```
If the custom type is a class with private members that must be serialized, the `serialize` template function has to be either declared inside the custom type or defined as a friend when declared `extern`:
```
class SOME_TYPE {
	template<typename Archive>
	friend void serialize(Archive& aArchive, SOME_TYPE& aValue);
}
```
Raw arrays in custom types must be wrapped by a `gvk::serializer::BinaryData` type prior to being passed to the `Archive`. `gkv::serializer` provides a static convenience function for this operation:
```
struct Meshlet {
	unsigned int vertices[64];
	unsigned int indices[126][3];
	unsigned char triangle_count;
	unsigned char vertex_count;
};

template<typename Archive>
void serialize(Archive& aArchive, Meshlet& aValue)
{
	aArchive(gvk::serializer::binary_data(aValue.vertices, sizeof(Meshlet::vertices)),
			 gvk::serializer::binary_data(aValue.indices, sizeof(Meshlet::indices)),
			 aValue.triangle_count,
			 aValue.vertex_count);
}
```
For custom serialization function examples see [`serializer.hpp#L259`](https://github.com/cg-tuwien/Gears-Vk/blob/master/framework/include/serializer.hpp#L259) and following.
