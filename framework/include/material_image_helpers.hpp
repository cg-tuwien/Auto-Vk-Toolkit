#pragma once

namespace cgb
{	
	extern std::optional<command_buffer> copy_image_to_another(image_t& aSrcImage, image_t& aDstImage, sync aSyncHandler = sync::wait_idle(), bool aRestoreSrcLayout = true, bool aRestoreDstLayout = true);
	extern std::optional<command_buffer> blit_image(image_t& aSrcImage, image_t& aDstImage, sync aSyncHandler = sync::wait_idle(), bool aRestoreSrcLayout = true, bool aRestoreDstLayout = true);

	template <typename Bfr>
	std::optional<command_buffer> copy_buffer_to_image(const Bfr& pSrcBuffer, const image_t& pDstImage, sync aSyncHandler = sync::wait_idle())
	{
		aSyncHandler.set_queue_hint(cgb::context().transfer_queue());
		
		auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
		// Sync before:
		aSyncHandler.establish_barrier_before_the_operation(pipeline_stage::transfer, read_memory_access{memory_access::transfer_read_access});

		// Operation:
		auto copyRegion = vk::BufferImageCopy()
			.setBufferOffset(0)
			// The bufferRowLength and bufferImageHeight fields specify how the pixels are laid out in memory. For example, you could have some padding 
			// bytes between rows of the image. Specifying 0 for both indicates that the pixels are simply tightly packed like they are in our case. [3]
			.setBufferRowLength(0)
			.setBufferImageHeight(0)
			.setImageSubresource(vk::ImageSubresourceLayers()
				.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setMipLevel(0u)
				.setBaseArrayLayer(0u)
				.setLayerCount(1u))
			.setImageOffset({ 0u, 0u, 0u })
			.setImageExtent(pDstImage.config().extent);
		commandBuffer.handle().copyBufferToImage(
			pSrcBuffer->buffer_handle(),
			pDstImage.handle(),
			vk::ImageLayout::eTransferDstOptimal,
			{ copyRegion });

		// Sync after:
		aSyncHandler.establish_barrier_after_the_operation(pipeline_stage::transfer, write_memory_access{memory_access::transfer_write_access});

		// Finish him:
		return aSyncHandler.submit_and_sync();
	}

	static ak::image create_1px_texture(std::array<uint8_t, 4> aColor, ak::memory_usage aMemoryUsage = ak::memory_usage::device, ak::image_usage aImageUsage = ak::image_usage::general_texture, sync aSyncHandler = sync::wait_idle())
	{
		aSyncHandler.set_queue_hint(cgb::context().transfer_queue());
		auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
		aSyncHandler.establish_barrier_before_the_operation(pipeline_stage::transfer, read_memory_access{memory_access::transfer_read_access});
		
		auto stagingBuffer = cgb::create_and_fill(
			cgb::generic_buffer_meta::create_from_size(sizeof(aColor)),
			ak::memory_usage::host_coherent,
			aColor.data(),
			sync::not_required(),
			vk::BufferUsageFlagBits::eTransferSrc);

		vk::Format selectedFormat = settings::gLoadImagesInSrgbFormatByDefault ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Unorm;

		auto img = ak::image_t::create(1, 1, selectedFormat, 1, aMemoryUsage, aImageUsage);
		auto finalTargetLayout = img->target_layout(); // save for later, because first, we need to transfer something into it
		
		// 1. Transition image layout to eTransferDstOptimal
		img->transition_to_layout(vk::ImageLayout::eTransferDstOptimal, sync::auxiliary_with_barriers(aSyncHandler, {}, {})); // no need for additional sync

		// 2. Copy buffer to image
		copy_buffer_to_image(stagingBuffer, img, sync::auxiliary_with_barriers(aSyncHandler, {}, {})); // There should be no need to make any memory available or visible, the transfer-execution dependency chain should be fine
																									   // TODO: Verify the above ^ comment
		commandBuffer.set_custom_deleter([lOwnedStagingBuffer=std::move(stagingBuffer)](){});
		
		// 3. Generate MIP-maps/transition to target layout:
		if (img->config().mipLevels > 1u) {
			// generat_mip_maps will perform the final layout transitiion
			img->set_target_layout(finalTargetLayout);
			img->generate_mip_maps(cgb::sync::auxiliary_with_barriers(aSyncHandler,
				// We have to sync copy_buffer_to_image with generate_mip_maps:
				[&img](command_buffer_t& cb, pipeline_stage dstStage, std::optional<read_memory_access> dstAccess){
					cb.establish_image_memory_barrier_rw(img,
						pipeline_stage::transfer, /* transfer -> transfer */ dstStage,
						write_memory_access{ memory_access::transfer_write_access }, /* -> */ dstAccess
					);
				},
				cgb::sync::steal_after_handler_immediately) // We know for sure that generate_mip_maps will invoke establish_barrier_after_the_operation => let's delegate that
			);
		}
		else {
			img->transition_to_layout(finalTargetLayout, sync::auxiliary_with_barriers(aSyncHandler, {}, {}));
			aSyncHandler.establish_barrier_after_the_operation(pipeline_stage::transfer, write_memory_access{memory_access::transfer_write_access}); 
		}

		auto result = aSyncHandler.submit_and_sync();
		assert (!result.has_value());
		
		return img;
	}

	static ak::image create_image_from_file(const std::string& aPath, vk::Format aFormat, ak::memory_usage aMemoryUsage = ak::memory_usage::device, ak::image_usage aImageUsage = ak::image_usage::general_texture, sync aSyncHandler = sync::wait_idle())
	{
		generic_buffer stagingBuffer;
		int width = 0;
		int height = 0;

		// ============ RGB 8-bit formats ==========
		if (ak::is_uint8_format(aFormat) || ak::is_int8_format(aFormat)) {

			stbi_set_flip_vertically_on_load(true);
			int desiredColorChannels = STBI_rgb_alpha;
			
			if (!ak::is_4component_format(aFormat)) { 
				if (ak::is_3component_format(aFormat)) {
					desiredColorChannels = STBI_rgb;
				}
				else if (ak::is_2component_format(aFormat)) {
					desiredColorChannels = STBI_grey_alpha;
				}
				else if (ak::is_1component_format(aFormat)) {
					desiredColorChannels = STBI_grey;
				}
			}
			
			int channelsInFile = 0;
			stbi_uc* pixels = stbi_load(aPath.c_str(), &width, &height, &channelsInFile, desiredColorChannels); 
			size_t imageSize = static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(desiredColorChannels);

			if (!pixels) {
				throw cgb::runtime_error(fmt::format("Couldn't load image from '{}' using stbi_load", aPath));
			}

			stagingBuffer = cgb::create_and_fill(
				cgb::generic_buffer_meta::create_from_size(imageSize),
				ak::memory_usage::host_coherent,
				pixels,
				sync::not_required(),
				vk::BufferUsageFlagBits::eTransferSrc);

			stbi_image_free(pixels);
		}
		// ============ RGB 16-bit float formats (HDR) ==========
		else if (is_float16_format(aFormat)) {
			
			stbi_set_flip_vertically_on_load(true);
			int desiredColorChannels = STBI_rgb_alpha;
			
			if (!is_4component_format(aFormat)) { 
				if (is_3component_format(aFormat)) {
					desiredColorChannels = STBI_rgb;
				}
				else if (is_2component_format(aFormat)) {
					desiredColorChannels = STBI_grey_alpha;
				}
				else if (is_1component_format(aFormat)) {
					desiredColorChannels = STBI_grey;
				}
			}

			int channelsInFile = 0;
			float* pixels = stbi_loadf(aPath.c_str(), &width, &height, &channelsInFile, desiredColorChannels); 
			size_t imageSize = static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(desiredColorChannels);

			if (!pixels) {
				throw cgb::runtime_error(fmt::format("Couldn't load image from '{}' using stbi_loadf", aPath));
			}

			stagingBuffer = cgb::create_and_fill(
				cgb::generic_buffer_meta::create_from_size(imageSize),
				ak::memory_usage::host_coherent,
				pixels,
				sync::not_required(),
				vk::BufferUsageFlagBits::eTransferSrc);

			stbi_image_free(pixels);
		}
		// ========= TODO: Support DDS loader, maybe also further loaders
		else {
			throw cgb::runtime_error("No loader for the given image format implemented.");
		}

		aSyncHandler.set_queue_hint(cgb::context().transfer_queue());
		auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
		aSyncHandler.establish_barrier_before_the_operation(pipeline_stage::transfer, read_memory_access{memory_access::transfer_read_access});

		auto img = cgb::image_t::create(width, height, cgb::image_format(aFormat), 1, aMemoryUsage, aImageUsage);
		auto finalTargetLayout = img->target_layout(); // save for later, because first, we need to transfer something into it

		// 1. Transition image layout to eTransferDstOptimal
		img->transition_to_layout(vk::ImageLayout::eTransferDstOptimal, sync::auxiliary_with_barriers(aSyncHandler, {}, {})); // no need for additional sync
		// TODO: The original implementation transitioned into cgb::image_format(_Format) format here, not to eTransferDstOptimal => Does it still work? If so, eTransferDstOptimal is fine.
		
		// 2. Copy buffer to image
		copy_buffer_to_image(stagingBuffer, img, sync::auxiliary_with_barriers(aSyncHandler, {}, {}));  // There should be no need to make any memory available or visible, the transfer-execution dependency chain should be fine
																										// TODO: Verify the above ^ comment
		commandBuffer.set_custom_deleter([lOwnedStagingBuffer=std::move(stagingBuffer)](){});
		
		// 3. Transition image layout to its target layout and handle lifetime of things via sync
		img->transition_to_layout(finalTargetLayout, sync::auxiliary_with_barriers(aSyncHandler, {}, {}));

		img->generate_mip_maps(cgb::sync::auxiliary_with_barriers(aSyncHandler, {}, {}));

		aSyncHandler.establish_barrier_after_the_operation(pipeline_stage::transfer, write_memory_access{memory_access::transfer_write_access});
		auto result = aSyncHandler.submit_and_sync();
		assert(!result.has_value());
		return img;
	}
	
	static image create_image_from_file(const std::string& aPath, bool aLoadHdrIfPossible = true, bool aLoadSrgbIfApplicable = true, int aPreferredNumberOfTextureComponents = 4, ak::memory_usage aMemoryUsage = ak::memory_usage::device, ak::image_usage aImageUsage = ak::image_usage::general_texture, sync aSyncHandler = sync::wait_idle())
	{
		cgb::image_format imFmt;
		if (aLoadHdrIfPossible) {
			if (stbi_is_hdr(aPath.c_str())) {
				switch (aPreferredNumberOfTextureComponents) {
				case 4:
					imFmt = image_format::default_rgb16f_4comp_format();
					break;
				// Attention: There's a high likelihood that your GPU does not support formats with less than four color components!
				case 3:
					imFmt = image_format::default_rgb16f_3comp_format();
					break;
				case 2:
					imFmt = image_format::default_rgb16f_2comp_format();
					break;
				case 1:
					imFmt = image_format::default_rgb16f_1comp_format();
					break;
				default:
					imFmt = image_format::default_rgb16f_4comp_format();
					break;
				}
				return create_image_from_file(aPath, imFmt, aMemoryUsage, aImageUsage, std::move(aSyncHandler));
			}
		}
		if (aLoadSrgbIfApplicable && settings::gLoadImagesInSrgbFormatByDefault) {
			switch (aPreferredNumberOfTextureComponents) {
			case 4:
				imFmt = cgb::default_srgb_4comp_format();
				break;
			// Attention: There's a high likelihood that your GPU does not support formats with less than four color components!
			case 3:
				imFmt = cgb::default_srgb_3comp_format();
				break;
			case 2:
				imFmt = cgb::default_srgb_2comp_format();
				break;
			case 1:
				imFmt = cgb::default_srgb_1comp_format();
				break;
			default:
				imFmt = cgb::default_srgb_4comp_format();
				break;
			}
			return create_image_from_file(aPath, imFmt, aMemoryUsage, aImageUsage, std::move(aSyncHandler));
		}
		switch (aPreferredNumberOfTextureComponents) {
		case 4:
			imFmt = cgb::default_rgb8_4comp_format();
			break;
		// Attention: There's a high likelihood that your GPU does not support formats with less than four color components!
		case 3:
			imFmt = cgb::default_rgb8_3comp_format();
			break;
		case 2:
			imFmt = cgb::default_rgb8_2comp_format();
			break;
		case 1:
			imFmt = cgb::default_rgb8_1comp_format();
			break;
		default:
			imFmt = cgb::default_rgb8_4comp_format();
			break;
		}
		return create_image_from_file(aPath, imFmt, aMemoryUsage, aImageUsage, std::move(aSyncHandler));
	}
	
	extern std::tuple<std::vector<material_gpu_data>, std::vector<ak::image_sampler>> convert_for_gpu_usage(
		std::vector<cgb::material_config> aMaterialConfigs, 
		ak::image_usage aImageUsage = ak::image_usage::general_texture,
		ak::filter_mode aTextureFilterMode = ak::filter_mode::bilinear,
		ak::border_handling_mode aBorderHandlingMode = ak::border_handling_mode::repeat,
		sync aSyncHandler = sync::wait_idle());

	template <typename... Rest>
	void add_tuple_or_indices(std::vector<std::tuple<std::reference_wrapper<const cgb::model_t>, std::vector<size_t>>>& aResult)
	{ }
	
	template <typename... Rest>
	void add_tuple_or_indices(std::vector<std::tuple<std::reference_wrapper<const cgb::model_t>, std::vector<size_t>>>& aResult, const model_t& aModel, const Rest&... rest)
	{
		aResult.emplace_back(std::cref(aModel), std::vector<size_t>{});
		add_tuple_or_indices(aResult, rest...);
	}

	template <typename... Rest>
	void add_tuple_or_indices(std::vector<std::tuple<std::reference_wrapper<const cgb::model_t>, std::vector<size_t>>>& aResult, size_t aMeshIndex, const Rest&... rest)
	{
		std::get<std::vector<size_t>>(aResult.back()).emplace_back(aMeshIndex);
		add_tuple_or_indices(aResult, rest...);
	}
	
	template <typename... Rest>
	void add_tuple_or_indices(std::vector<std::tuple<std::reference_wrapper<const cgb::model_t>, std::vector<size_t>>>& aResult, std::vector<size_t> aMeshIndices, const Rest&... rest)
	{
		auto& idxes = std::get<std::vector<size_t>>(aResult.back());
		idxes.insert(std::end(idxes), std::begin(aMeshIndices), std::end(aMeshIndices));
		add_tuple_or_indices(aResult, rest...);
	}
	
	template <typename... Args>
	std::vector<std::tuple<std::reference_wrapper<const cgb::model_t>, std::vector<size_t>>> make_models_and_meshes_selection(const Args&... args)
	{
		std::vector<std::tuple<std::reference_wrapper<const cgb::model_t>, std::vector<size_t>>> result;
		add_tuple_or_indices(result, args...);
		return result;
	}
	
	extern std::tuple<std::vector<glm::vec3>, std::vector<uint32_t>> get_vertices_and_indices(const std::vector<std::tuple<std::reference_wrapper<const cgb::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes);
	extern std::tuple<vertex_buffer, index_buffer> create_vertex_and_index_buffers(const std::vector<std::tuple<std::reference_wrapper<const cgb::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, sync aSyncHandler = sync::wait_idle());
	extern std::vector<glm::vec3> get_normals(const std::vector<std::tuple<std::reference_wrapper<const cgb::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes);
	extern vertex_buffer create_normals_buffer(const std::vector<std::tuple<std::reference_wrapper<const cgb::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, sync aSyncHandler = sync::wait_idle());
	extern std::vector<glm::vec3> get_tangents(const std::vector<std::tuple<std::reference_wrapper<const cgb::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes);
	extern vertex_buffer create_tangents_buffer(const std::vector<std::tuple<std::reference_wrapper<const cgb::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, sync aSyncHandler = sync::wait_idle());
	extern std::vector<glm::vec3> get_bitangents(const std::vector<std::tuple<std::reference_wrapper<const cgb::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes);
	extern vertex_buffer create_bitangents_buffer(const std::vector<std::tuple<std::reference_wrapper<const cgb::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, sync aSyncHandler = sync::wait_idle());
	extern std::vector<glm::vec4> get_colors(const std::vector<std::tuple<std::reference_wrapper<const cgb::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, int _ColorsSet);
	extern vertex_buffer create_colors_buffer(const std::vector<std::tuple<std::reference_wrapper<const cgb::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, int _ColorsSet = 0, sync aSyncHandler = sync::wait_idle());
	extern std::vector<glm::vec2> get_2d_texture_coordinates(const std::vector<std::tuple<std::reference_wrapper<const cgb::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet);
	extern vertex_buffer create_2d_texture_coordinates_buffer(const std::vector<std::tuple<std::reference_wrapper<const cgb::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet = 0, sync aSyncHandler = sync::wait_idle());
	extern std::vector<glm::vec3> get_3d_texture_coordinates(const std::vector<std::tuple<std::reference_wrapper<const cgb::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet);
	extern vertex_buffer create_3d_texture_coordinates_buffer(const std::vector<std::tuple<std::reference_wrapper<const cgb::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet = 0, sync aSyncHandler = sync::wait_idle());

	// TODO: Not sure if the following leads somewhere:
	//
	//using model_ref_getter_func = std::function<const model_t&()>;
	//using mesh_indices_getter_func = std::function<std::vector<size_t>()>;

	//inline static void merge_distinct_materials_into(std::unordered_map<material_config, std::vector<std::tuple<model_ref_getter_func, mesh_indices_getter_func>>>& _Target)
	//{ }

	//inline static void merge_distinct_materials_into(std::unordered_map<material_config, std::vector<std::tuple<model_ref_getter_func, mesh_indices_getter_func>>>& _Target, const material_config& _Material, const model_t& _Model, const std::vector<size_t>& _Indices)
	//{
	//	_Target[_Material].push_back(std::make_tuple(
	//		[modelAddr = &_Model]() -> const model_t& { return *modelAddr; },
	//		[meshIndices = _Indices]() -> std::vector<size_t> { return meshIndices; }
	//	));
	//}

	//inline static void merge_distinct_materials_into(std::unordered_map<material_config, std::vector<std::tuple<model_ref_getter_func, mesh_indices_getter_func>>>& _Target, const material_config& _Material, const orca_scene_t& _OrcaScene, const std::tuple<size_t, std::vector<size_t>>& _ModelAndMeshIndices)
	//{
	//	_Target[_Material].push_back(std::make_tuple(
	//		[modelAddr = &static_cast<const model_t&>(_OrcaScene.model_at_index(std::get<0>(_ModelAndMeshIndices)).mLoadedModel)]() -> const model_t& { return *modelAddr; },
	//		[meshIndices = std::get<1>(_ModelAndMeshIndices)]() -> std::vector<size_t> { return meshIndices; }
	//	));
	//}

	//template <typename O, typename D, typename... Os, typename... Ds>
	//void merge_distinct_materials_into(std::unordered_map<material_config, std::vector<std::tuple<model_ref_getter_func, mesh_indices_getter_func>>>& _Target, std::tuple<const O&, const D&> _First, std::tuple<const Os&, const Ds&>... _Rest) 
	//{
	//	merge_distinct_materials_into(_Target, )
	//	
	//}

	//template <typename... Os, typename... Ds>
	//std::unordered_map<material_config, std::vector<std::tuple<model_ref_getter_func, mesh_indices_getter_func>>> merge_distinct_materials(std::tuple<const Os&, const Ds&>... _Rest) 
	//{
	//	std::unordered_map<material_config, std::vector<std::tuple<model_ref_getter_func, mesh_indices_getter_func>>> result;
	//	
	//}
}
