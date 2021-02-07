# Serializer
Gears-Vk features an object serializer to improve load times of huge 3D models or ORCA scenes by
serializing all the processed and ready to use data into a cache file during the first run. In
further runs, the data can be directly deserialized into types and structures used by Gears-Vk
and expensive parsing and processing of huge models or scenes is avoided.

### Serializer modes
* `gvk::serialize(const std::string&)`: Creates an object for serializing into the given file path
* `gvk::deserialize(const std::string&)`: Creates an object for deserializing from the given file
  path
* `gvk::serializer(gvk::serialize&&)`: Initialize for serialization
* `gvk::serializer(gvk::deserialize&&)`: Initialize for deserialization

### Serialization/Deserialization functions
* `gvk::serializer::archive(Type&&)`: General function for types (classes, structs, int, float, etc.)
* `gvk::serializer::archive_memory(Type&&, size_t)`: Specialized function for a fixed sized block of memory
* `gvk::serializer::archive_buffer(avk::resource_reference<avk::buffer_t>)`: Specialized function
  for `avk::buffer`


### Helper functions

#### Check if the serializer already created a **cache** file
* `gvk::does_cache_file_exist(const std::string_view)`

#### Various \*\_cached variations of orca scene loading functions to ease serializer usage and simplify code
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

# How to use
The serializer is available via `gvk::serializer` class and can be used for, i.e. improving load
times of huge orca scenes. It is initialized either by a `gvk::serializer::serialize` or a
`gvk::serializer::deserialize` object, where both take a path to a **cache** file.
`gvk::serializer::serialize` creates and writes to the **cache** file and
`gvk::serializer::deserialize` reads from the **cache** file at the given path. A helper function
`gvk::does_cache_file_exist(const std::string_view)` can be used to determine which initialization
method shall be used.
```
const std::string cacheFilePath(pathToOrcaScene + ".cache");

auto serializer = gvk::does_cache_file_exist(cacheFilePath) ?
	gvk::serializer(gvk::serializer::deserialize(cacheFilePath)) :
	gvk::serializer(gvk::serializer::serialize(cacheFilePath));
```
After initialization the current **mode** of the serializer can be retrived by calling `mode()` on
the serializer, which either returns **serializer::mode::serialize** or
**serializer::mode::deserialize**. To serialize or deserialize a type, the `archive(Type&&)`
function template is called on the serializer object. This function serializes the given object to
the **cache** file if the mode of the serialize is **serialize** or deserializes the object from the
**cache** file into the given object if the mode is **deserialize**. The following code shows how a
typical serializer usage can look like when used during orca scene loading.
```
std::vector<glm::vec3> positions;
if (serializer.mode() == gvk::serializer::mode::serialize) {
	get_positions(orca_scene, positions);
}
serializer.archive(positions);

size_t numPositions = (serializer.mode() == gvk::serializer::mode::serialize) ? positions.size() : 0;
serializer.archive(numPositions);
```

The serializer features spezialised functions for serializing and deserializing a block of memory
and host visible buffers. Both can be used separately but are especially usefull when used in
conjunction to, i.e. serialize/deserialize image data. Image data loaded from a file can be
serialized using `archive_memory`
```
size_t imageSize;
// Load image frome file
void* pixels = load_image(pathToImage, &imageSize);

serializer.archive_memory(pixels, imageSize);
```
before it is copied into a host visible GPU buffer. In further program runs, i.e. when
deserializing, the serializer can copy the image data directly from the cached file into a host
visible GPU buffer using `archive_buffer`
```
// Create host visible staging buffer for filling on host side
auto sb = avk::create_buffer(
	AVK_STAGING_BUFFER_MEMORY_USAGE,
	vk::BufferUsageFlagBits::eTransferSrc,
	avk::generic_buffer_meta::create_from_size(imageSize)
);

aSerializer.archive_buffer(sb);
```
which avoids an extra memory allocation in main memory for the image data before the copy into a host
visible GPU buffer.

## \*\_cached functions

Gears-Vk features \*\_cached function variants of various helper functions for orca scene loading to
ease serializer usage and avoid constant writing of different code paths for both modes of the
serializer. For example, to retrieve a 2D texture coordinates buffer for model data
(**modelAndMeshes**), one can use
```
auto texCoordsBuffer = gvk::create_2d_texture_coordinates_buffer_cached(
	serializer, modelAndMeshes, 0, avk::sync::wait_idle()
);
```
instead of the longer version
```
avk::buffer texCoordsBuffer;
if (serializer.mode() == gvk::serializer::mode::serialize) {
	texCoordsBuffer = gvk::create_2d_texture_coordinates_buffer(
		modelAndMeshes, 0, avk::sync::wait_idle()
	);
}
serializer.archive_buffer(texCoordsBuffer);
```
The \*\_cached functions internaly take case of either serializing or deserialising the necessary
data (in above example the content of the returned buffer, i.e. the 2D texture coordinates).
For further \*\_cached function usage see `void load_orca_scene_cached(...)` in the **orca_loader** example at [`orca_loader.cpp#L187`](https://github.com/cg-tuwien/Gears-Vk/blob/master/examples/orca_loader/source/orca_loader.cpp#L187)

## Custom type serialization
Every type needs a special template function which defines how a type is serialized. Often used
types such as `glm::vec3`, `glm::mat4`, etc. and various `std` types this special function is
already defined in **serializer.hpp**. To add serialization ability to a custom type, a
**serialize** function template in the same namespace as the custom type must be created. The
function template takes two arguments, an **Archive** and your custom **Type** as reference, and
passes every member of the **Type** to the **Archive**. If one of the members is also a custom
type, a separate **serialize** function template must be created.
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
If the custom type is a class with private members that must be serialized, the **serialize**
template function has to be either declared inside the custom type or defined as a friend when
declared extern:
```
class SOME_TYPE {
	template<typename Archive>
	friend void serialize(Archive& aArchive, SOME_TYPE& aValue);
}
```
For custom serialization function examples see **serializer** at [`serializer.hpp#L259`](https://github.com/cg-tuwien/Gears-Vk/blob/master/framework/include/serializer.hpp#L259)
