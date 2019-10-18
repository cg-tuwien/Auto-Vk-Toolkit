#pragma once

namespace cgb
{	
	extern vk::ImageMemoryBarrier create_image_barrier(vk::Image pImage, vk::Format pFormat, vk::AccessFlags pSrcAccessMask, vk::AccessFlags pDstAccessMask, vk::ImageLayout pOldLayout, vk::ImageLayout pNewLayout, std::optional<vk::ImageSubresourceRange> pSubresourceRange = std::nullopt);

	extern void transition_image_layout(image_t& _Image, vk::Format _Format, vk::ImageLayout _NewLayout, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler = {}, std::vector<semaphore> _WaitSemaphores = {});

	extern void copy_image_to_another(const image_t& pSrcImage, image_t& pDstImage, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler = {}, std::vector<semaphore> _WaitSemaphores = {});

	template <typename Bfr>
	owning_resource<semaphore_t> copy_buffer_to_image(const Bfr& pSrcBuffer, const image_t& pDstImage, std::vector<semaphore> _WaitSemaphores = {})
	{
		//auto commandBuffer = context().create_command_buffers_for_transfer(1);
		auto commandBuffer = cgb::context().transfer_queue().pool().get_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

		// Immediately start recording the command buffer:
		commandBuffer.begin_recording();

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
			pDstImage.image_handle(),
			vk::ImageLayout::eTransferDstOptimal,
			{ copyRegion });

		// That's all
		commandBuffer.end_recording();

		// Create a semaphore which can, or rather, MUST be used to wait for the results
		auto copyCompleteSemaphore = cgb::context().transfer_queue().submit_and_handle_with_semaphore(std::move(commandBuffer), std::move(_WaitSemaphores));
		// copyCompleteSemaphore->set_semaphore_wait_stage(...); TODO: Set wait stage
		return copyCompleteSemaphore;
	}

	static image create_1px_texture(std::array<uint8_t, 4> _Color, cgb::memory_usage _MemoryUsage = cgb::memory_usage::device, cgb::image_usage _ImageUsage = cgb::image_usage::versatile_image, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler = {}, std::vector<semaphore> _WaitSemaphores = {})
	{
		auto stagingBuffer = cgb::create_and_fill(
			cgb::generic_buffer_meta::create_from_size(sizeof(_Color)),
			cgb::memory_usage::host_coherent,
			_Color.data(),
			{}, {},
			vk::BufferUsageFlagBits::eTransferSrc);

		vk::Format selectedFormat = settings::gLoadImagesInSrgbFormatByDefault ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Unorm;

		auto img = cgb::image_t::create(1, 1, cgb::image_format(selectedFormat), false, 1, _MemoryUsage, _ImageUsage);
		// 1. Transition image layout to eTransferDstOptimal
		cgb::transition_image_layout(img, selectedFormat, vk::ImageLayout::eTransferDstOptimal, [&](semaphore sem1) {
			// 2. Copy buffer to image
			auto sem2 = cgb::copy_buffer_to_image(stagingBuffer, img, cgb::make_vector(std::move(sem1)));
			// 3. Transition image layout to its target layout and handle the semaphore(s) and resources
			cgb::transition_image_layout(img, selectedFormat, img->target_layout(), [&](semaphore sem3) {
				sem3->set_custom_deleter([
					ownBuffer = std::move(stagingBuffer)
				](){});
				handle_semaphore(std::move(sem3), std::move(_SemaphoreHandler));
			}, cgb::make_vector(std::move(sem2)));
		}, std::move(_WaitSemaphores));
		
		return img;
	}

	static image create_image_from_file(const std::string& _Path, std::optional<cgb::image_format> _Format = {}, cgb::memory_usage _MemoryUsage = cgb::memory_usage::device, cgb::image_usage _ImageUsage = cgb::image_usage::versatile_image, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler = {}, std::vector<semaphore> _WaitSemaphores = {})
	{
		int width, height, channelsInFile;
		stbi_set_flip_vertically_on_load(true);

		// 4component textures by default!
		int channelsInImage = STBI_rgb_alpha;
		if (_Format.has_value() && !is_4component_format(_Format.value())) {
			// Attention: There's a high likelihood that your GPU does not support formats with less than four color components!
			if (is_3component_format(_Format.value())) {
				channelsInImage = STBI_rgb;
			}
			else if (is_2component_format(_Format.value())) {
				channelsInImage = STBI_grey_alpha;
			}
			else if (is_1component_format(_Format.value())) {
				channelsInImage = STBI_grey;
			}
		}
		
		stbi_uc* pixels = stbi_load(_Path.c_str(), &width, &height, &channelsInFile, channelsInImage); // TODO: Support different formats!
		size_t imageSize = width * height * channelsInImage;

		if (!pixels) {
			throw std::runtime_error(fmt::format("Couldn't load image from '{}' using stbi_load", _Path));
		}

		vk::Format selectedOrDemandedFormat;
		if (_Format.has_value()) {
			selectedOrDemandedFormat = _Format->mFormat;
		}
		else {
			// Set a default format if none was specified:
			switch (channelsInImage) {
			case 4:
				selectedOrDemandedFormat = settings::gLoadImagesInSrgbFormatByDefault ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Unorm;
				break;
			case 3:
				selectedOrDemandedFormat = settings::gLoadImagesInSrgbFormatByDefault ? vk::Format::eR8G8B8Srgb : vk::Format::eR8G8B8Unorm;
				break;
			case 2:
				selectedOrDemandedFormat = settings::gLoadImagesInSrgbFormatByDefault ? vk::Format::eR8G8Srgb : vk::Format::eR8G8Unorm;
				break;
			case 1:
				selectedOrDemandedFormat = settings::gLoadImagesInSrgbFormatByDefault ? vk::Format::eR8Srgb : vk::Format::eR8Unorm;
				break;
			default:
				throw std::runtime_error(fmt::format("Unexpected number of channels ({}).", channelsInImage));
			}
		}

		auto stagingBuffer = cgb::create_and_fill(
			cgb::generic_buffer_meta::create_from_size(imageSize),
			cgb::memory_usage::host_coherent,
			pixels,
			{}, {},
			vk::BufferUsageFlagBits::eTransferSrc);
			
		stbi_image_free(pixels);

		auto img = cgb::image_t::create(width, height, cgb::image_format(selectedOrDemandedFormat), false, 1, _MemoryUsage, _ImageUsage);
		// 1. Transition image layout to eTransferDstOptimal
		cgb::transition_image_layout(img, selectedOrDemandedFormat, vk::ImageLayout::eTransferDstOptimal, [&](semaphore sem1) {
			// 2. Copy buffer to image
			auto sem2 = cgb::copy_buffer_to_image(stagingBuffer, img, cgb::make_vector(std::move(sem1)));
			// 3. Transition image layout to its target layout and handle the semaphore(s) and resources
			cgb::transition_image_layout(img, selectedOrDemandedFormat, img->target_layout(), [&](semaphore sem3) {
				sem3->set_custom_deleter([
					ownBuffer = std::move(stagingBuffer)
				](){});
				handle_semaphore(std::move(sem3), std::move(_SemaphoreHandler));
			}, cgb::make_vector(std::move(sem2)));
		}, std::move(_WaitSemaphores));
		
		return img;
	}

	extern std::tuple<std::vector<material_gpu_data>, std::vector<image_sampler>> convert_for_gpu_usage(std::vector<cgb::material_config> _MaterialConfigs, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler = {}, cgb::image_usage _ImageUsage = cgb::image_usage::read_only_sampled_image);

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
