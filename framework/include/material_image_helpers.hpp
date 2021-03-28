#pragma once
#include <gvk.hpp>

namespace gvk
{
	static avk::image create_1px_texture_cached(std::array<uint8_t, 4> aColor, vk::Format aFormat = vk::Format::eR8G8B8A8Unorm, avk::memory_usage aMemoryUsage = avk::memory_usage::device, avk::image_usage aImageUsage = avk::image_usage::general_texture, avk::sync aSyncHandler = avk::sync::wait_idle(), std::optional<std::reference_wrapper<gvk::serializer>> aSerializer = {})
	{
		auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
		aSyncHandler.establish_barrier_before_the_operation(avk::pipeline_stage::transfer, avk::read_memory_access{avk::memory_access::transfer_read_access});

		auto stagingBuffer = context().create_buffer(
			AVK_STAGING_BUFFER_MEMORY_USAGE,
			vk::BufferUsageFlagBits::eTransferSrc,
			avk::generic_buffer_meta::create_from_size(sizeof(aColor))
		);
		if (!aSerializer) {
			stagingBuffer->fill(aColor.data(), 0, avk::sync::not_required());
		}
		else if (aSerializer && aSerializer->get().mode() == gvk::serializer::mode::serialize) {
			stagingBuffer->fill(aColor.data(), 0, avk::sync::not_required());
			aSerializer->get().archive_memory(aColor.data(), sizeof(aColor));
		}
		else if (aSerializer && aSerializer->get().mode() == gvk::serializer::mode::deserialize) {
			aSerializer->get().archive_buffer(stagingBuffer);
		}

		auto img = context().create_image(1u, 1u, aFormat, 1, aMemoryUsage, aImageUsage);
		auto finalTargetLayout = img->target_layout(); // save for later, because first, we need to transfer something into it

		// 1. Transition image layout to eTransferDstOptimal
		img->transition_to_layout(vk::ImageLayout::eTransferDstOptimal, avk::sync::auxiliary_with_barriers(aSyncHandler, {}, {})); // no need for additional sync

		// 2. Copy buffer to image
		copy_buffer_to_image(avk::const_referenced(stagingBuffer), avk::referenced(img), {}, avk::sync::auxiliary_with_barriers(aSyncHandler, {}, {})); // There should be no need to make any memory available or visible, the transfer-execution dependency chain should be fine
																									   // TODO: Verify the above ^ comment
		commandBuffer.set_custom_deleter([lOwnedStagingBuffer=std::move(stagingBuffer)](){});

		// 3. Generate MIP-maps/transition to target layout:
		if (img->config().mipLevels > 1u) {
			// generat_mip_maps will perform the final layout transitiion
			img->set_target_layout(finalTargetLayout);
			img->generate_mip_maps(avk::sync::auxiliary_with_barriers(aSyncHandler,
				// We have to sync copy_buffer_to_image with generate_mip_maps:
				[&img](avk::command_buffer_t& cb, avk::pipeline_stage dstStage, std::optional<avk::read_memory_access> dstAccess){
					cb.establish_image_memory_barrier_rw(img.get(),
						avk::pipeline_stage::transfer, /* transfer -> transfer */ dstStage,
						avk::write_memory_access{ avk::memory_access::transfer_write_access }, /* -> */ dstAccess
					);
				},
				avk::sync::steal_after_handler_immediately) // We know for sure that generate_mip_maps will invoke establish_barrier_after_the_operation => let's delegate that
			);
		}
		else {
			img->transition_to_layout(finalTargetLayout, avk::sync::auxiliary_with_barriers(aSyncHandler, {}, {}));
			aSyncHandler.establish_barrier_after_the_operation(avk::pipeline_stage::transfer, avk::write_memory_access{avk::memory_access::transfer_write_access});
		}

		auto result = aSyncHandler.submit_and_sync();
		assert (!result.has_value());

		return img;
	}

	static avk::image create_1px_texture(std::array<uint8_t, 4> aColor, vk::Format aFormat = vk::Format::eR8G8B8A8Unorm, avk::memory_usage aMemoryUsage = avk::memory_usage::device, avk::image_usage aImageUsage = avk::image_usage::general_texture, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		return create_1px_texture_cached(aColor, aFormat, aMemoryUsage, aImageUsage, std::move(aSyncHandler));
	}

	static avk::image create_1px_texture_cached(gvk::serializer& aSerializer, std::array<uint8_t, 4> aColor, vk::Format aFormat = vk::Format::eR8G8B8A8Unorm, avk::memory_usage aMemoryUsage = avk::memory_usage::device, avk::image_usage aImageUsage = avk::image_usage::general_texture, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		return create_1px_texture_cached(aColor, aFormat, aMemoryUsage, aImageUsage, std::move(aSyncHandler), aSerializer);
	}

	// create image_data from texture files
	static image_data create_image_resource(const std::string& aPath, bool aLoadHdrIfPossible = true, bool aLoadSrgbIfApplicable = true, bool aFlip = true,
		int aPreferredNumberOfTextureComponents = 4)
	{
		image_data_t result(aPath, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents);

		return result;
	}

	static image_data create_image_resource(const std::vector<std::string>& aPaths, bool aLoadHdrIfPossible = true, bool aLoadSrgbIfApplicable = true, bool aFlip = true,
		int aPreferredNumberOfTextureComponents = 4)
	{
		image_data_t result(aPaths, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents);

		return result;
	}

	// create cubemap from an image resource
	avk::image create_cubemap_from_image_resource_cached(avk::resource_reference<image_data_t> aImageResource, avk::memory_usage aMemoryUsage = avk::memory_usage::device,
		avk::image_usage aImageUsage = avk::image_usage::general_cube_map_texture, avk::sync aSyncHandler = avk::sync::wait_idle(), std::optional<std::reference_wrapper<gvk::serializer>> aSerializer = {});

	static avk::image create_cubemap_from_image_resource_cached(gvk::serializer& aSerializer, avk::resource_reference<image_data_t> aImageResource, avk::memory_usage aMemoryUsage = avk::memory_usage::device, 
		avk::image_usage aImageUsage = avk::image_usage::general_cube_map_texture, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		return create_cubemap_from_image_resource_cached(aImageResource, aMemoryUsage, aImageUsage, std::move(aSyncHandler), aSerializer);
	}

	static avk::image create_cubemap_from_image_resource(avk::resource_reference<image_data_t> aImageResource, avk::memory_usage aMemoryUsage = avk::memory_usage::device,
		avk::image_usage aImageUsage = avk::image_usage::general_cube_map_texture, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		return create_cubemap_from_image_resource_cached(aImageResource, aMemoryUsage, aImageUsage, std::move(aSyncHandler));
	}

	// create cubemap from a single file
	avk::image create_cubemap_from_file_cached(const std::string& aPath, bool aLoadHdrIfPossible = true, bool aLoadSrgbIfApplicable = true, bool aFlip = true,
		int aPreferredNumberOfTextureComponents = 4, avk::memory_usage aMemoryUsage = avk::memory_usage::device,
		avk::image_usage aImageUsage = avk::image_usage::general_cube_map_texture, avk::sync aSyncHandler = avk::sync::wait_idle(), std::optional<std::reference_wrapper<gvk::serializer>> aSerializer = {});

	static avk::image create_cubemap_from_file_cached(gvk::serializer& aSerializer, const std::string& aPath, bool aLoadHdrIfPossible = true, bool aLoadSrgbIfApplicable = true, bool aFlip = true,
		int aPreferredNumberOfTextureComponents = 4, avk::memory_usage aMemoryUsage = avk::memory_usage::device,
		avk::image_usage aImageUsage = avk::image_usage::general_cube_map_texture, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		return create_cubemap_from_file_cached(aPath, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents, aMemoryUsage, aImageUsage, std::move(aSyncHandler), aSerializer);
	}

	static avk::image create_cubemap_from_file(const std::string& aPath, bool aLoadHdrIfPossible = true, bool aLoadSrgbIfApplicable = true, bool aFlip = true,
		int aPreferredNumberOfTextureComponents = 4, avk::memory_usage aMemoryUsage = avk::memory_usage::device,
		avk::image_usage aImageUsage = avk::image_usage::general_cube_map_texture, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		return create_cubemap_from_file_cached(aPath, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents, aMemoryUsage, aImageUsage, std::move(aSyncHandler));
	}

	// create cubemap from six individual files
	avk::image create_cubemap_from_file_cached(const std::vector<std::string>& aPaths, bool aLoadHdrIfPossible = true, bool aLoadSrgbIfApplicable = true, bool aFlip = true,
		int aPreferredNumberOfTextureComponents = 4, avk::memory_usage aMemoryUsage = avk::memory_usage::device,
		avk::image_usage aImageUsage = avk::image_usage::general_cube_map_texture, avk::sync aSyncHandler = avk::sync::wait_idle(), std::optional<std::reference_wrapper<gvk::serializer>> aSerializer = {});

	static avk::image create_cubemap_from_file_cached(gvk::serializer& aSerializer, const std::vector<std::string>& aPaths, bool aLoadHdrIfPossible = true, bool aLoadSrgbIfApplicable = true, bool aFlip = true, int aPreferredNumberOfTextureComponents = 4, avk::memory_usage aMemoryUsage = avk::memory_usage::device, avk::image_usage aImageUsage = avk::image_usage::general_cube_map_texture, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		return create_cubemap_from_file_cached(aPaths, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents, aMemoryUsage, aImageUsage, std::move(aSyncHandler), aSerializer);
	}

	static avk::image create_cubemap_from_file(const std::vector<std::string>& aPaths, bool aLoadHdrIfPossible = true, bool aLoadSrgbIfApplicable = true, bool aFlip = true, int aPreferredNumberOfTextureComponents = 4, avk::memory_usage aMemoryUsage = avk::memory_usage::device, avk::image_usage aImageUsage = avk::image_usage::general_cube_map_texture, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		return create_cubemap_from_file_cached(aPaths, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents, aMemoryUsage, aImageUsage, std::move(aSyncHandler));
	}

	// create image from an image resource
	avk::image create_image_from_image_resource_cached(avk::resource_reference<image_data_t> aImageResource, avk::memory_usage aMemoryUsage = avk::memory_usage::device,
		avk::image_usage aImageUsage = avk::image_usage::general_texture, avk::sync aSyncHandler = avk::sync::wait_idle(), std::optional<std::reference_wrapper<gvk::serializer>> aSerializer = {});

	static avk::image create_image_from_image_resource_cached(gvk::serializer& aSerializer, avk::resource_reference<image_data_t> aImageResource, avk::memory_usage aMemoryUsage = avk::memory_usage::device,
		avk::image_usage aImageUsage = avk::image_usage::general_texture, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		return create_image_from_image_resource_cached(aImageResource, aMemoryUsage, aImageUsage, std::move(aSyncHandler), aSerializer);
	}

	static avk::image create_image_from_image_resource(avk::resource_reference<image_data_t> aImageResource, avk::memory_usage aMemoryUsage = avk::memory_usage::device,
		avk::image_usage aImageUsage = avk::image_usage::general_texture, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		return create_image_from_image_resource_cached(aImageResource, aMemoryUsage, aImageUsage, std::move(aSyncHandler));
	}

	// create image from a file
	avk::image create_image_from_file_cached(const std::string& aPath, bool aLoadHdrIfPossible = true, bool aLoadSrgbIfApplicable = true, bool aFlip = true, int aPreferredNumberOfTextureComponents = 4, avk::memory_usage aMemoryUsage = avk::memory_usage::device, avk::image_usage aImageUsage = avk::image_usage::general_texture, avk::sync aSyncHandler = avk::sync::wait_idle(), std::optional<std::reference_wrapper<gvk::serializer>> aSerializer = {});

	static avk::image create_image_from_file_cached(gvk::serializer& aSerializer, const std::string& aPath, bool aLoadHdrIfPossible = true, bool aLoadSrgbIfApplicable = true, bool aFlip = true, int aPreferredNumberOfTextureComponents = 4, avk::memory_usage aMemoryUsage = avk::memory_usage::device, avk::image_usage aImageUsage = avk::image_usage::general_texture, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		return create_image_from_file_cached(aPath, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents, aMemoryUsage, aImageUsage, std::move(aSyncHandler), aSerializer);
	}

	static avk::image create_image_from_file(const std::string& aPath, bool aLoadHdrIfPossible = true, bool aLoadSrgbIfApplicable = true, bool aFlip = true, int aPreferredNumberOfTextureComponents = 4, avk::memory_usage aMemoryUsage = avk::memory_usage::device, avk::image_usage aImageUsage = avk::image_usage::general_texture, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		return create_image_from_file_cached(aPath, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents, aMemoryUsage, aImageUsage, std::move(aSyncHandler));
	}

	/**	Takes a vector of gvk::material_config elements and converts it into a format that is usable
	 *	in shaders. Concretely, this means that each input gvk::material_config is transformed into
	 *	a gvk::material_gpu_data struct. The latter no longer contains the paths to images, but
	 *	instead, indices to image samplers.
	 *	The image samplers referenced by those indices are returned as the second tuple element.
	 *
	 *	Whenever textures are not set in the input gvk::material_config elements, they will be
	 *	replaced by "dummy textures" which are sized 1x1 and contain a single value. There are two
	 *	types of such replacement textures:
	 *	- 1x1 pure white (i.e. unorm values of (1,1,1,1))
	 *	- 1x1 "straight up normal" texture containing byte values (127, 127, 255, 0)
	 *
	 *	Either 0, 1, or 2 such automatically created textures can be created and returned.
	 *	To find out how many such 1x1 textures actually were created, you can use the following code:
	 *	(Although it is not 100% reliable (if the first regular texture is sized 1x1) but for most
	 *	 real-world cases, it should give the right result.)
	 *
	 *		int numAutoGen = 0;
	 *		for (int i = 0; i < std::min(2, static_cast<int>(imageSamplers.size())); ++i) {
	 *			auto e = imageSamplers[i]->get_image_view()->get_image().config().extent;
	 *			if (e.width == 1u && e.height == 1u) {
	 *				++numAutoGen;
	 *			}
	 *		}
	 *
	 *	@param	aMaterialConfigs		A vector of multiple gvk::material_config entries that are to
	 *									be converted into vectors of gvk::material_gpu_data and avk::image_sampler
	 *	@param	aLoadTexturesInSrgb		If true, "diffuse textures", "ambient textures", and "extra textures"
	 *									are assumed to be in sRGB format and will be loaded as such.
	 *									All other textures will always be loaded in non-sRGB format.
	 *	@param	aFlipTextures			Flip the images loaded from file vertically.
	 *	@param	aImageUsage				Image usage for all the textures that are loaded.
	 *	@param	aTextureFilterMode		Texture filter mode for all the textures that are loaded.
	 *	@param	aBorderHandlingMode		Border handling mode for all the textures that are loaded.
	 *	@param	aSyncHandler			How to synchronize the GPU-upload of texture memory.
	 *	@return	A tuple of two elements: The first element contains a vector of gvk::material_gpu_data
	 *			entries, which are gvk::material_config entries converted into a format suitable to be
	 *			used in UBOs or SSBOs, and the second element contains a vector of avk::image_samplers,
	 *			containing all the "combined image samplers" for all the textures which are referenced
	 *			from the gvk::material_gpu_data entries from the first tuple element. Also the second
	 *			tuple element is suitable to be bound and used in GPU shaders as is.
	 */
	extern std::tuple<std::vector<material_gpu_data>, std::vector<avk::image_sampler>> convert_for_gpu_usage(
		const std::vector<gvk::material_config>& aMaterialConfigs,
		bool aLoadTexturesInSrgb = false,
		bool aFlipTextures = false,
		avk::image_usage aImageUsage = avk::image_usage::general_texture,
		avk::filter_mode aTextureFilterMode = avk::filter_mode::trilinear,
		avk::border_handling_mode aBorderHandlingMode = avk::border_handling_mode::repeat,
		avk::sync aSyncHandler = avk::sync::wait_idle());

	template <typename... Rest>
	void add_tuple_or_indices(std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aResult)
	{ }

	template <typename... Rest>
	void add_tuple_or_indices(std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aResult, const model& aModel, const Rest&... rest)
	{
		aResult.emplace_back(avk::const_referenced(aModel), std::vector<size_t>{});
		add_tuple_or_indices(aResult, rest...);
	}

	template <typename... Rest>
	void add_tuple_or_indices(std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aResult, size_t aMeshIndex, const Rest&... rest)
	{
		std::get<std::vector<size_t>>(aResult.back()).emplace_back(aMeshIndex);
		add_tuple_or_indices(aResult, rest...);
	}

	template <typename... Rest>
	void add_tuple_or_indices(std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aResult, std::vector<size_t> aMeshIndices, const Rest&... rest)
	{
		auto& idxes = std::get<std::vector<size_t>>(aResult.back());
		idxes.insert(std::end(idxes), std::begin(aMeshIndices), std::end(aMeshIndices));
		add_tuple_or_indices(aResult, rest...);
	}

	template <typename... Args>
	std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>> make_models_and_meshes_selection(const Args&... args)
	{
		std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>> result;
		add_tuple_or_indices(result, args...);
		return result;
	}
	
	extern std::tuple<std::vector<glm::vec3>, std::vector<uint32_t>> get_vertices_and_indices(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes);
	extern std::tuple<avk::buffer, avk::buffer> create_vertex_and_index_buffers(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, vk::BufferUsageFlags aUsageFlags = {}, avk::sync aSyncHandler = avk::sync::wait_idle());
	extern std::vector<glm::vec3> get_normals(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes);
	extern avk::buffer create_normals_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, avk::sync aSyncHandler = avk::sync::wait_idle());
	extern std::vector<glm::vec3> get_tangents(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes);
	extern avk::buffer create_tangents_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, avk::sync aSyncHandler = avk::sync::wait_idle());
	extern std::vector<glm::vec3> get_bitangents(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes);
	extern avk::buffer create_bitangents_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, avk::sync aSyncHandler = avk::sync::wait_idle());
	extern std::vector<glm::vec4> get_colors(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aColorsSet);
	extern avk::buffer create_colors_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aColorsSet = 0, avk::sync aSyncHandler = avk::sync::wait_idle());
	extern std::vector<glm::vec4> get_bone_weights(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, bool aNormalizeBoneWeights = false);
	extern avk::buffer create_bone_weights_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, avk::sync aSyncHandler = avk::sync::wait_idle());
	extern avk::buffer create_bone_weights_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, bool aNormalizeBoneWeights, avk::sync aSyncHandler = avk::sync::wait_idle());
	extern std::vector<glm::uvec4> get_bone_indices(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aBoneIndexOffset = 0u);
	extern avk::buffer create_bone_indices_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aBoneIndexOffset = 0u, avk::sync aSyncHandler = avk::sync::wait_idle());
	extern std::vector<glm::uvec4> get_bone_indices_for_single_target_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aInitialBoneIndexOffset = 0u);
	extern avk::buffer create_bone_indices_for_single_target_buffer_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aInitialBoneIndexOffset = 0u, avk::sync aSyncHandler = avk::sync::wait_idle());
	extern std::vector<glm::uvec4> get_bone_indices_for_single_target_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, const std::vector<mesh_index_t>& aReferenceMeshIndices);
	extern avk::buffer create_bone_indices_for_single_target_buffer_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, const std::vector<mesh_index_t>& aReferenceMeshIndices, avk::sync aSyncHandler = avk::sync::wait_idle());
	extern std::vector<glm::vec2> get_2d_texture_coordinates(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet);
	extern avk::buffer create_2d_texture_coordinates_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet = 0, avk::sync aSyncHandler = avk::sync::wait_idle());
	extern std::vector<glm::vec2> get_2d_texture_coordinates_flipped(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet);
	extern avk::buffer create_2d_texture_coordinates_flipped_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet = 0, avk::sync aSyncHandler = avk::sync::wait_idle());
	extern std::vector<glm::vec3> get_3d_texture_coordinates(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet);
	extern avk::buffer create_3d_texture_coordinates_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet = 0, avk::sync aSyncHandler = avk::sync::wait_idle());

	/** *cached versions for serialization */
	extern std::tuple<std::vector<glm::vec3>, std::vector<uint32_t>> get_vertices_and_indices_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes);
	extern std::tuple<avk::buffer, avk::buffer> create_vertex_and_index_buffers_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, vk::BufferUsageFlags aUsageFlags = {}, avk::sync aSyncHandler = avk::sync::wait_idle());
	extern std::vector<glm::vec3> get_normals_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes);
	extern avk::buffer create_normals_buffer_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, avk::sync aSyncHandler = avk::sync::wait_idle());
	extern std::vector<glm::vec3> get_tangents_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes);
	extern avk::buffer create_tangents_buffer_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, avk::sync aSyncHandler = avk::sync::wait_idle());
	extern std::vector<glm::vec3> get_bitangents_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes);
	extern avk::buffer create_bitangents_buffer_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, avk::sync aSyncHandler = avk::sync::wait_idle());
	extern std::vector<glm::vec4> get_colors_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aColorsSet);
	extern avk::buffer create_colors_buffer_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aColorsSet = 0, avk::sync aSyncHandler = avk::sync::wait_idle());
	extern std::vector<glm::vec4> get_bone_weights_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, bool aNormalizeBoneWeights = false);
	extern avk::buffer create_bone_weights_buffer_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, avk::sync aSyncHandler = avk::sync::wait_idle());
	extern avk::buffer create_bone_weights_buffer_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, bool aNormalizeBoneWeights, avk::sync aSyncHandler = avk::sync::wait_idle());
	extern std::vector<glm::uvec4> get_bone_indices_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aBoneIndexOffset = 0u);
	extern avk::buffer create_bone_indices_buffer_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aBoneIndexOffset = 0u, avk::sync aSyncHandler = avk::sync::wait_idle());
	extern std::vector<glm::uvec4> get_bone_indices_for_single_target_buffer_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aInitialBoneIndexOffset = 0u);
	extern avk::buffer create_bone_indices_for_single_target_buffer_buffer_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aInitialBoneIndexOffset = 0u, avk::sync aSyncHandler = avk::sync::wait_idle());
	extern std::vector<glm::uvec4> get_bone_indices_for_single_target_buffer_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, const std::vector<mesh_index_t>& aReferenceMeshIndices);
	extern avk::buffer create_bone_indices_for_single_target_buffer_buffer_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, const std::vector<mesh_index_t>& aReferenceMeshIndices, avk::sync aSyncHandler = avk::sync::wait_idle());
	extern std::vector<glm::vec2> get_2d_texture_coordinates_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet);
	extern avk::buffer create_2d_texture_coordinates_buffer_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet = 0, avk::sync aSyncHandler = avk::sync::wait_idle());
	extern std::vector<glm::vec2> get_2d_texture_coordinates_flipped_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet);
	extern avk::buffer create_2d_texture_coordinates_flipped_buffer_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet = 0, avk::sync aSyncHandler = avk::sync::wait_idle());
	extern std::vector<glm::vec3> get_3d_texture_coordinates_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet);
	extern avk::buffer create_3d_texture_coordinates_buffer_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet = 0, avk::sync aSyncHandler = avk::sync::wait_idle());

	extern std::tuple<std::vector<material_gpu_data>, std::vector<avk::image_sampler>> convert_for_gpu_usage_cached(
		gvk::serializer& aSerializer,
		const std::vector<gvk::material_config>& aMaterialConfigs,
		bool aLoadTexturesInSrgb = false,
		bool aFlipTextures = false,
		avk::image_usage aImageUsage = avk::image_usage::general_texture,
		avk::filter_mode aTextureFilterMode = avk::filter_mode::trilinear,
		avk::border_handling_mode aBorderHandlingMode = avk::border_handling_mode::repeat,
		avk::sync aSyncHandler = avk::sync::wait_idle());

}
