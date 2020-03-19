#pragma once

namespace cgb
{	
	extern vk::ImageMemoryBarrier create_image_barrier(vk::Image pImage, vk::Format pFormat, vk::AccessFlags pSrcAccessMask, vk::AccessFlags pDstAccessMask, vk::ImageLayout pOldLayout, vk::ImageLayout pNewLayout, std::optional<vk::ImageSubresourceRange> pSubresourceRange = std::nullopt);

	extern void transition_image_layout(image_t& aImage, vk::Format aFormat, vk::ImageLayout aNewLayout, sync aSyncHandler = sync::wait_idle());

	extern void copy_image_to_another(const image_t& pSrcImage, image_t& pDstImage, sync aSyncHandler = sync::wait_idle());

	template <typename Bfr>
	void copy_buffer_to_image(const Bfr& pSrcBuffer, const image_t& pDstImage, sync aSyncHandler = sync::wait_idle())
	{
		//auto commandBuffer = context().create_command_buffers_for_transfer(1);
		auto commandBuffer = cgb::context().transfer_queue().create_single_use_command_buffer();

		// Immediately start recording the command buffer:
		commandBuffer->begin_recording();

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

		commandBuffer->handle().copyBufferToImage(
			pSrcBuffer->buffer_handle(),
			pDstImage.image_handle(),
			vk::ImageLayout::eTransferDstOptimal,
			{ copyRegion });

		// That's all
		aSyncHandler.set_sync_stages_and_establish_barrier(commandBuffer, pipeline_stage::transfer, memory_access::transfer);
		commandBuffer->end_recording();
		aSyncHandler.submit_and_sync(std::move(commandBuffer));
	}

	

	static image create_1px_texture(std::array<uint8_t, 4> _Color, cgb::memory_usage _MemoryUsage = cgb::memory_usage::device, cgb::image_usage _ImageUsage = cgb::image_usage::versatile_image, sync aSyncHandler = sync::wait_idle())
	{
		auto stagingBuffer = cgb::create_and_fill(
			cgb::generic_buffer_meta::create_from_size(sizeof(_Color)),
			cgb::memory_usage::host_coherent,
			_Color.data(),
			{}, {},
			vk::BufferUsageFlagBits::eTransferSrc);

		vk::Format selectedFormat = settings::gLoadImagesInSrgbFormatByDefault ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Unorm;

		auto img = cgb::image_t::create(1, 1, cgb::image_format(selectedFormat), false, 1, _MemoryUsage, _ImageUsage);
		auto finalTargetLayout = img->target_layout(); // store for later, because first, we need to transfer something into it
		
		// 1. Transition image layout to eTransferDstOptimal
		img->transition_to_layout(vk::ImageLayout::eTransferDstOptimal, sync::with_barriers_subordinate(aSyncHandler, {}, {}, true)); // no need for additional sync

		// 2. Copy buffer to image
		copy_buffer_to_image(stagingBuffer, img, sync::with_barrier_subordinate(aSyncHandler,
			[&img](command_buffer_t& cb, pipeline_stage srcStage, std::optional<memory_access> srcAccess){
				// I have been provided with src*, my task is to fill in dst* (i.e. what comes after)
				cb.establish_image_memory_barrier(img, 
					srcStage, pipeline_stage::transfer, 
					srcAccess, // make the transferred memory available
					{}); // no need to make any memory visible
			}
			// no need to sync before
		));
		
		// 3. Transition image layout to its target layout and handle the semaphore(s) and resources
		img->transition_to_layout(finalTargetLayout, std::move(aSyncHandler));
		
		return img;
	}

	static image create_image_from_file(const std::string& _Path, cgb::image_format _Format, cgb::memory_usage _MemoryUsage = cgb::memory_usage::device, cgb::image_usage _ImageUsage = cgb::image_usage::versatile_image, sync aSyncHandler = sync::wait_idle())
	{
		generic_buffer stagingBuffer;
		int width = 0;
		int height = 0;

		// ============ RGB 8-bit formats ==========
		if (is_uint8_format(_Format) || is_int8_format(_Format)) {

			stbi_set_flip_vertically_on_load(true);
			int desiredColorChannels = STBI_rgb_alpha;
			
			if (!is_4component_format(_Format)) { 
				if (is_3component_format(_Format)) {
					desiredColorChannels = STBI_rgb;
				}
				else if (is_2component_format(_Format)) {
					desiredColorChannels = STBI_grey_alpha;
				}
				else if (is_1component_format(_Format)) {
					desiredColorChannels = STBI_grey;
				}
			}
			
			int channelsInFile = 0;
			stbi_uc* pixels = stbi_load(_Path.c_str(), &width, &height, &channelsInFile, desiredColorChannels); 
			size_t imageSize = width * height * desiredColorChannels;

			if (!pixels) {
				throw std::runtime_error(fmt::format("Couldn't load image from '{}' using stbi_load", _Path));
			}

			stagingBuffer = cgb::create_and_fill(
				cgb::generic_buffer_meta::create_from_size(imageSize),
				cgb::memory_usage::host_coherent,
				pixels,
				{}, {},
				vk::BufferUsageFlagBits::eTransferSrc);

			stbi_image_free(pixels);
		}
		// ============ RGB 16-bit float formats (HDR) ==========
		else if (is_float16_format(_Format)) {
			
			stbi_set_flip_vertically_on_load(true);
			int desiredColorChannels = STBI_rgb_alpha;
			
			if (!is_4component_format(_Format)) { 
				if (is_3component_format(_Format)) {
					desiredColorChannels = STBI_rgb;
				}
				else if (is_2component_format(_Format)) {
					desiredColorChannels = STBI_grey_alpha;
				}
				else if (is_1component_format(_Format)) {
					desiredColorChannels = STBI_grey;
				}
			}

			int channelsInFile = 0;
			float* pixels = stbi_loadf(_Path.c_str(), &width, &height, &channelsInFile, desiredColorChannels); 
			size_t imageSize = width * height * desiredColorChannels;

			if (!pixels) {
				throw std::runtime_error(fmt::format("Couldn't load image from '{}' using stbi_loadf", _Path));
			}

			stagingBuffer = cgb::create_and_fill(
				cgb::generic_buffer_meta::create_from_size(imageSize),
				cgb::memory_usage::host_coherent,
				pixels,
				{}, {},
				vk::BufferUsageFlagBits::eTransferSrc);

			stbi_image_free(pixels);
		}
		// ========= TODO: Support DDS loader, maybe also further loaders
		else {
			throw std::runtime_error("No loader for the given image format implemented.");
		}

		auto img = cgb::image_t::create(width, height, cgb::image_format(_Format), false, 1, _MemoryUsage, _ImageUsage);
		// 1. Transition image layout to eTransferDstOptimal
		transition_image_layout(img, _Format.mFormat, vk::ImageLayout::eTransferDstOptimal, sync::with_barrier_subordinate(aSyncHandler));
		// 2. Copy buffer to image
		copy_buffer_to_image(stagingBuffer, img, sync::with_barrier_subordinate(aSyncHandler));
		// 3. Transition image layout to its target layout and handle the semaphore(s) and resources
		transition_image_layout(img, _Format.mFormat, img->target_layout(), sync::with_barrier_subordinate(aSyncHandler));
		
		return img;
	}
	
	static image create_image_from_file(const std::string& _Path, bool _LoadHdrIfPossible = true, bool _LoadSrgbIfApplicable = true, int _PreferredNumberOfTextureComponents = 4, cgb::memory_usage _MemoryUsage = cgb::memory_usage::device, cgb::image_usage _ImageUsage = cgb::image_usage::versatile_image, sync aSyncHandler = sync::wait_idle())
	{
		cgb::image_format imFmt;
		if (_LoadHdrIfPossible) {
			if (stbi_is_hdr(_Path.c_str())) {
				switch (_PreferredNumberOfTextureComponents) {
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
				return create_image_from_file(_Path, imFmt, _MemoryUsage, _ImageUsage, std::move(aSyncHandler));
			}
		}
		if (_LoadSrgbIfApplicable && settings::gLoadImagesInSrgbFormatByDefault) {
			switch (_PreferredNumberOfTextureComponents) {
			case 4:
				imFmt = image_format::default_srgb_4comp_format();
				break;
			// Attention: There's a high likelihood that your GPU does not support formats with less than four color components!
			case 3:
				imFmt = image_format::default_srgb_3comp_format();
				break;
			case 2:
				imFmt = image_format::default_srgb_2comp_format();
				break;
			case 1:
				imFmt = image_format::default_srgb_1comp_format();
				break;
			default:
				imFmt = image_format::default_srgb_4comp_format();
				break;
			}
			return create_image_from_file(_Path, imFmt, _MemoryUsage, _ImageUsage, std::move(aSyncHandler));
		}
		switch (_PreferredNumberOfTextureComponents) {
		case 4:
			imFmt = image_format::default_rgb8_4comp_format();
			break;
		// Attention: There's a high likelihood that your GPU does not support formats with less than four color components!
		case 3:
			imFmt = image_format::default_rgb8_3comp_format();
			break;
		case 2:
			imFmt = image_format::default_rgb8_2comp_format();
			break;
		case 1:
			imFmt = image_format::default_rgb8_1comp_format();
			break;
		default:
			imFmt = image_format::default_rgb8_4comp_format();
			break;
		}
		return create_image_from_file(_Path, imFmt, _MemoryUsage, _ImageUsage, std::move(aSyncHandler));
	}
	
	extern std::tuple<std::vector<material_gpu_data>, std::vector<image_sampler>> convert_for_gpu_usage(
		std::vector<cgb::material_config> _MaterialConfigs, 
		std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler = {}, 
		cgb::image_usage _ImageUsage = cgb::image_usage::read_only_sampled_image,
		cgb::filter_mode _TextureFilterMode = cgb::filter_mode::bilinear, // TODO: Implement MIP-mapping and default to anisotropic 16x
		cgb::border_handling_mode _BorderHandlingMode = cgb::border_handling_mode::repeat);

	extern std::tuple<std::vector<glm::vec3>, std::vector<uint32_t>> get_combined_vertices_and_indices_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes);
	extern std::tuple<vertex_buffer, index_buffer> get_combined_vertex_and_index_buffers_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler = {}, std::vector<semaphore> _WaitSemaphores = {});
	extern std::vector<glm::vec3> get_combined_normals_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes);
	extern vertex_buffer get_combined_normal_buffers_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler = {}, std::vector<semaphore> _WaitSemaphores = {});
	extern std::vector<glm::vec3> get_combined_tangents_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes);
	extern vertex_buffer get_combined_tangent_buffers_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler = {}, std::vector<semaphore> _WaitSemaphores = {});
	extern std::vector<glm::vec3> get_combined_bitangents_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes);
	extern vertex_buffer get_combined_bitangent_buffers_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler = {}, std::vector<semaphore> _WaitSemaphores = {});
	extern std::vector<glm::vec4> get_combined_colors_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes, int _ColorsSet);
	extern vertex_buffer get_combined_color_buffers_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes, int _ColorsSet = 0, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler = {}, std::vector<semaphore> _WaitSemaphores = {});
	extern std::vector<glm::vec2> get_combined_2d_texture_coordinates_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes, int _TexCoordSet);
	extern vertex_buffer get_combined_2d_texture_coordinate_buffers_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes, int _TexCoordSet = 0, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler = {}, std::vector<semaphore> _WaitSemaphores = {});
	extern std::vector<glm::vec3> get_combined_3d_texture_coordinates_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes, int _TexCoordSet);
	extern vertex_buffer get_combined_3d_texture_coordinate_buffers_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes, int _TexCoordSet = 0, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler = {}, std::vector<semaphore> _WaitSemaphores = {});

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
