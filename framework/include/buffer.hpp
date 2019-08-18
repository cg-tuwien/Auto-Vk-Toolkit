 #pragma once

namespace cgb
{
	// Forward declare templated cgb::buffer and set up some type defs.
	// The definition of the buffer will always be inside the context-specific implementation files.
	template <typename Meta>
	class buffer_t;

	using generic_buffer_t = buffer_t<generic_buffer_meta>;
	using uniform_buffer_t = buffer_t<uniform_buffer_meta>;
	using uniform_texel_buffer_t = buffer_t<uniform_texel_buffer_meta>;
	using storage_buffer_t = buffer_t<storage_buffer_meta>;
	using storage_texel_buffer_t = buffer_t<storage_texel_buffer_meta>;
	using vertex_buffer_t = buffer_t<vertex_buffer_meta>;
	using index_buffer_t = buffer_t<index_buffer_meta>;
	using instance_buffer_t = buffer_t<instance_buffer_meta>;

	using generic_buffer		= owning_resource<generic_buffer_t>;
	using uniform_buffer		= owning_resource<uniform_buffer_t>;
	using uniform_texel_buffer	= owning_resource<uniform_texel_buffer_t>;
	using storage_buffer		= owning_resource<storage_buffer_t>;
	using storage_texel_buffer	= owning_resource<storage_texel_buffer_t>;
	using vertex_buffer			= owning_resource<vertex_buffer_t>;
	using index_buffer			= owning_resource<index_buffer_t>;
	using instance_buffer		= owning_resource<instance_buffer_t>;

	//using buffer = std::variant<
	//	generic_buffer,
	//	uniform_buffer,
	//	uniform_texel_buffer,
	//	storage_buffer,
	//	storage_texel_buffer,
	//	vertex_buffer,
	//	index_buffer
	//>;

	//using buffers = std::variant<
	//	std::vector<generic_buffer>,
	//	std::vector<uniform_buffer>,
	//	std::vector<uniform_texel_buffer>,
	//	std::vector<storage_buffer>,
	//	std::vector<storage_texel_buffer>,
	//	std::vector<vertex_buffer>,
	//	std::vector<index_buffer>
	//>;

	///** Gets a reference to the data of a specific element at the given index, stored in 
	// *	a collection of variants, regardless of how it is stored/referenced there, be 
	// *	it stored directly or referenced via a smart pointer.
	// */
	//template <typename T, typename V>
	//T& get_at(V v, size_t index)	{
	//	return get<T>(v[index]);
	//}
}
