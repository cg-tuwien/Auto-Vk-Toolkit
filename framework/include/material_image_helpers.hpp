#pragma once
#include <gvk.hpp>

namespace gvk
{	
	static avk::image create_1px_texture(std::array<uint8_t, 4> aColor, vk::Format aFormat = vk::Format::eR8G8B8A8Unorm, avk::memory_usage aMemoryUsage = avk::memory_usage::device, avk::image_usage aImageUsage = avk::image_usage::general_texture, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
		aSyncHandler.establish_barrier_before_the_operation(avk::pipeline_stage::transfer, avk::read_memory_access{avk::memory_access::transfer_read_access});

		auto stagingBuffer = context().create_buffer(
			avk::memory_usage::host_coherent,
			vk::BufferUsageFlagBits::eTransferSrc,
			avk::generic_buffer_meta::create_from_size(sizeof(aColor))
		);
		stagingBuffer->fill(aColor.data(), 0, avk::sync::not_required());

		auto img = context().create_image(1u, 1u, aFormat, 1, aMemoryUsage, aImageUsage);
		auto finalTargetLayout = img->target_layout(); // save for later, because first, we need to transfer something into it
		
		// 1. Transition image layout to eTransferDstOptimal
		img->transition_to_layout(vk::ImageLayout::eTransferDstOptimal, avk::sync::auxiliary_with_barriers(aSyncHandler, {}, {})); // no need for additional sync

		// 2. Copy buffer to image
		copy_buffer_to_image(stagingBuffer, img, avk::sync::auxiliary_with_barriers(aSyncHandler, {}, {})); // There should be no need to make any memory available or visible, the transfer-execution dependency chain should be fine
																									   // TODO: Verify the above ^ comment
		commandBuffer.set_custom_deleter([lOwnedStagingBuffer=std::move(stagingBuffer)](){});
		
		// 3. Generate MIP-maps/transition to target layout:
		if (img->config().mipLevels > 1u) {
			// generat_mip_maps will perform the final layout transitiion
			img->set_target_layout(finalTargetLayout);
			img->generate_mip_maps(avk::sync::auxiliary_with_barriers(aSyncHandler,
				// We have to sync copy_buffer_to_image with generate_mip_maps:
				[&img](avk::command_buffer_t& cb, avk::pipeline_stage dstStage, std::optional<avk::read_memory_access> dstAccess){
					cb.establish_image_memory_barrier_rw(img,
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

	static avk::image create_image_from_file(const std::string& aPath, vk::Format aFormat, avk::memory_usage aMemoryUsage = avk::memory_usage::device, avk::image_usage aImageUsage = avk::image_usage::general_texture, avk::sync aSyncHandler = avk::sync::wait_idle(), std::optional<gli::texture> aAlreadyLoadedGliTexture = {})
	{
		avk::buffer stagingBuffer;
		int width = 0;
		int height = 0;

		// ============ Compressed formats (DDS) ==========
		if (avk::is_block_compressed_format(aFormat)) {
			if (!aAlreadyLoadedGliTexture.has_value()) {
				aAlreadyLoadedGliTexture = gli::load(aPath);
			}
			auto& gliTex = aAlreadyLoadedGliTexture.value();

			if (gliTex.target() != gli::TARGET_2D) {
				throw gvk::runtime_error(fmt::format("The image '{}' is not intended to be used as 2D image. Can't load it.", aPath));
			}

			width  = gliTex.extent()[0];
			height = gliTex.extent()[1];

			stagingBuffer = context().create_buffer(
				avk::memory_usage::host_coherent,
				vk::BufferUsageFlagBits::eTransferSrc,
				avk::generic_buffer_meta::create_from_size(gliTex.size())
			);
			stagingBuffer->fill(gliTex.data(), 0, avk::sync::not_required());
		}
		// ============ RGB 8-bit formats ==========
		else if (avk::is_uint8_format(aFormat) || avk::is_int8_format(aFormat)) {

			stbi_set_flip_vertically_on_load(true);
			int desiredColorChannels = STBI_rgb_alpha;
			
			if (!avk::is_4component_format(aFormat)) { 
				if (avk::is_3component_format(aFormat)) {
					desiredColorChannels = STBI_rgb;
				}
				else if (avk::is_2component_format(aFormat)) {
					desiredColorChannels = STBI_grey_alpha;
				}
				else if (avk::is_1component_format(aFormat)) {
					desiredColorChannels = STBI_grey;
				}
			}
			
			int channelsInFile = 0;
			stbi_uc* pixels = stbi_load(aPath.c_str(), &width, &height, &channelsInFile, desiredColorChannels); 
			size_t imageSize = static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(desiredColorChannels);

			if (!pixels) {
				throw gvk::runtime_error(fmt::format("Couldn't load image from '{}' using stbi_load", aPath));
			}

			stagingBuffer = context().create_buffer(
				avk::memory_usage::host_coherent,
				vk::BufferUsageFlagBits::eTransferSrc,
				avk::generic_buffer_meta::create_from_size(imageSize)
			);
			stagingBuffer->fill(pixels, 0, avk::sync::not_required());

			stbi_image_free(pixels);
		}
		// ============ RGB 16-bit float formats (HDR) ==========
		else if (avk::is_float16_format(aFormat)) {
			
			stbi_set_flip_vertically_on_load(true);
			int desiredColorChannels = STBI_rgb_alpha;
			
			if (!avk::is_4component_format(aFormat)) { 
				if (avk::is_3component_format(aFormat)) {
					desiredColorChannels = STBI_rgb;
				}
				else if (avk::is_2component_format(aFormat)) {
					desiredColorChannels = STBI_grey_alpha;
				}
				else if (avk::is_1component_format(aFormat)) {
					desiredColorChannels = STBI_grey;
				}
			}

			int channelsInFile = 0;
			float* pixels = stbi_loadf(aPath.c_str(), &width, &height, &channelsInFile, desiredColorChannels); 
			size_t imageSize = static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(desiredColorChannels);

			if (!pixels) {
				throw gvk::runtime_error(fmt::format("Couldn't load image from '{}' using stbi_loadf", aPath));
			}

			stagingBuffer = context().create_buffer(
				avk::memory_usage::host_coherent,
				vk::BufferUsageFlagBits::eTransferSrc,
				avk::generic_buffer_meta::create_from_size(imageSize)
			);
			stagingBuffer->fill(pixels, 0, avk::sync::not_required());
			
			stbi_image_free(pixels);
		}
		else {
			throw gvk::runtime_error("No loader for the given image format implemented.");
		}

		auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
		aSyncHandler.establish_barrier_before_the_operation(avk::pipeline_stage::transfer, avk::read_memory_access{avk::memory_access::transfer_read_access});

		auto img = context().create_image(width, height, aFormat, 1, aMemoryUsage, aImageUsage);
		auto finalTargetLayout = img->target_layout(); // save for later, because first, we need to transfer something into it

		// 1. Transition image layout to eTransferDstOptimal
		img->transition_to_layout(vk::ImageLayout::eTransferDstOptimal, avk::sync::auxiliary_with_barriers(aSyncHandler, {}, {})); // no need for additional sync
		// TODO: The original implementation transitioned into cgb::image_format(_Format) format here, not to eTransferDstOptimal => Does it still work? If so, eTransferDstOptimal is fine.
		
		// 2. Copy buffer to image
		copy_buffer_to_image(stagingBuffer, img, avk::sync::auxiliary_with_barriers(aSyncHandler, {}, {}));  // There should be no need to make any memory available or visible, the transfer-execution dependency chain should be fine
																										// TODO: Verify the above ^ comment
		commandBuffer.set_custom_deleter([lOwnedStagingBuffer=std::move(stagingBuffer)](){});
		
		// 3. Transition image layout to its target layout and handle lifetime of things via sync
		img->transition_to_layout(finalTargetLayout, avk::sync::auxiliary_with_barriers(aSyncHandler, {}, {}));

		img->generate_mip_maps(avk::sync::auxiliary_with_barriers(aSyncHandler, {}, {}));

		aSyncHandler.establish_barrier_after_the_operation(avk::pipeline_stage::transfer, avk::write_memory_access{ avk::memory_access::transfer_write_access });
		auto result = aSyncHandler.submit_and_sync();
		assert(!result.has_value());
		return img;
	}
	
	static avk::image create_image_from_file(const std::string& aPath, bool aLoadHdrIfPossible = true, bool aLoadSrgbIfApplicable = true, int aPreferredNumberOfTextureComponents = 4, avk::memory_usage aMemoryUsage = avk::memory_usage::device, avk::image_usage aImageUsage = avk::image_usage::general_texture, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		std::optional<vk::Format> imFmt = {};

		std::optional<gli::texture> gliTex = gli::load(aPath);
		if (!gliTex.value().empty()) {
			auto gliFmt = gliTex.value().format();
			switch (gliFmt) {
			// See "Khronos Data Format Specification": https://www.khronos.org/registry/DataFormat/specs/1.3/dataformat.1.3.html#S3TC
			// And Vulkan specification: https://www.khronos.org/registry/vulkan/specs/1.2-khr-extensions/html/chap42.html#appendix-compressedtex-bc
			case gli::format::FORMAT_RGB_DXT1_UNORM_BLOCK8:
				imFmt = vk::Format::eBc1RgbUnormBlock;
				break;
			case gli::format::FORMAT_RGB_DXT1_SRGB_BLOCK8:
				imFmt = vk::Format::eBc1RgbSrgbBlock;
				break;
			case gli::format::FORMAT_RGBA_DXT1_UNORM_BLOCK8:
				imFmt = vk::Format::eBc1RgbaUnormBlock;
				break;
			case gli::format::FORMAT_RGBA_DXT1_SRGB_BLOCK8:
				imFmt = vk::Format::eBc1RgbaSrgbBlock;
				break;
			case gli::format::FORMAT_RGBA_DXT3_UNORM_BLOCK16:
				imFmt = vk::Format::eBc2UnormBlock;
				break;
			case gli::format::FORMAT_RGBA_DXT3_SRGB_BLOCK16:
				imFmt = vk::Format::eBc2SrgbBlock; 
				break;
			case gli::format::FORMAT_RGBA_DXT5_UNORM_BLOCK16:
				imFmt = vk::Format::eBc3UnormBlock;
				break;
			case gli::format::FORMAT_RGBA_DXT5_SRGB_BLOCK16:
				imFmt = vk::Format::eBc3SrgbBlock;
				break;
			case gli::format::FORMAT_R_ATI1N_UNORM_BLOCK8:
				imFmt = vk::Format::eBc4UnormBlock;
				break;
			// See "Khronos Data Format Specification": https://www.khronos.org/registry/DataFormat/specs/1.3/dataformat.1.3.html#RGTC
			// And Vulkan specification: https://www.khronos.org/registry/vulkan/specs/1.2-khr-extensions/html/chap42.html#appendix-compressedtex-bc
			case gli::format::FORMAT_R_ATI1N_SNORM_BLOCK8:
				imFmt = vk::Format::eBc4SnormBlock;
				break;
			case gli::format::FORMAT_RG_ATI2N_UNORM_BLOCK16:
				imFmt = vk::Format::eBc5UnormBlock;
				break;
			case gli::format::FORMAT_RG_ATI2N_SNORM_BLOCK16:
				imFmt = vk::Format::eBc5SnormBlock;
			}
		}
		else {
			gliTex.reset();
		}
		
		if (!imFmt.has_value() && aLoadHdrIfPossible) {
			if (stbi_is_hdr(aPath.c_str())) {
				switch (aPreferredNumberOfTextureComponents) {
				case 4:
					imFmt = default_rgb16f_4comp_format();
					break;
				// Attention: There's a high likelihood that your GPU does not support formats with less than four color components!
				case 3:
					imFmt = default_rgb16f_3comp_format();
					break;
				case 2:
					imFmt = default_rgb16f_2comp_format();
					break;
				case 1:
					imFmt = default_rgb16f_1comp_format();
					break;
				default:
					imFmt = default_rgb16f_4comp_format();
					break;
				}
			}
		}
		
		if (!imFmt.has_value() && aLoadSrgbIfApplicable) {
			switch (aPreferredNumberOfTextureComponents) {
			case 4:
				imFmt = gvk::default_srgb_4comp_format();
				break;
			// Attention: There's a high likelihood that your GPU does not support formats with less than four color components!
			case 3:
				imFmt = gvk::default_srgb_3comp_format();
				break;
			case 2:
				imFmt = gvk::default_srgb_2comp_format();
				break;
			case 1:
				imFmt = gvk::default_srgb_1comp_format();
				break;
			default:
				imFmt = gvk::default_srgb_4comp_format();
				break;
			}
		}

		if (!imFmt.has_value()) {
			switch (aPreferredNumberOfTextureComponents) {
			case 4:
				imFmt = gvk::default_rgb8_4comp_format();
				break;
			// Attention: There's a high likelihood that your GPU does not support formats with less than four color components!
			case 3:
				imFmt = gvk::default_rgb8_3comp_format();
				break;
			case 2:
				imFmt = gvk::default_rgb8_2comp_format();
				break;
			case 1:
				imFmt = gvk::default_rgb8_1comp_format();
				break;
			default:
				imFmt = gvk::default_rgb8_4comp_format();
				break;
			}
		}

		if (!imFmt.has_value()) {
			throw gvk::runtime_error(fmt::format("Could not determine the image format of image '{}'", aPath));
		}
		
		return create_image_from_file(aPath, imFmt.value(), aMemoryUsage, aImageUsage, std::move(aSyncHandler), std::move(gliTex));
	}
	
	extern std::tuple<std::vector<material_gpu_data>, std::vector<avk::image_sampler>> convert_for_gpu_usage(
		const std::vector<gvk::material_config>& aMaterialConfigs,
		bool aLoadTexturesInSrgb = false,
		avk::image_usage aImageUsage = avk::image_usage::general_texture,
		avk::filter_mode aTextureFilterMode = avk::filter_mode::bilinear,
		avk::border_handling_mode aBorderHandlingMode = avk::border_handling_mode::repeat,
		avk::sync aSyncHandler = avk::sync::wait_idle());

	template <typename... Rest>
	void add_tuple_or_indices(std::vector<std::tuple<std::reference_wrapper<const gvk::model_t>, std::vector<size_t>>>& aResult)
	{ }
	
	template <typename... Rest>
	void add_tuple_or_indices(std::vector<std::tuple<std::reference_wrapper<const gvk::model_t>, std::vector<size_t>>>& aResult, const model_t& aModel, const Rest&... rest)
	{
		aResult.emplace_back(std::cref(aModel), std::vector<size_t>{});
		add_tuple_or_indices(aResult, rest...);
	}

	template <typename... Rest>
	void add_tuple_or_indices(std::vector<std::tuple<std::reference_wrapper<const gvk::model_t>, std::vector<size_t>>>& aResult, size_t aMeshIndex, const Rest&... rest)
	{
		std::get<std::vector<size_t>>(aResult.back()).emplace_back(aMeshIndex);
		add_tuple_or_indices(aResult, rest...);
	}
	
	template <typename... Rest>
	void add_tuple_or_indices(std::vector<std::tuple<std::reference_wrapper<const gvk::model_t>, std::vector<size_t>>>& aResult, std::vector<size_t> aMeshIndices, const Rest&... rest)
	{
		auto& idxes = std::get<std::vector<size_t>>(aResult.back());
		idxes.insert(std::end(idxes), std::begin(aMeshIndices), std::end(aMeshIndices));
		add_tuple_or_indices(aResult, rest...);
	}
	
	template <typename... Args>
	std::vector<std::tuple<std::reference_wrapper<const gvk::model_t>, std::vector<size_t>>> make_models_and_meshes_selection(const Args&... args)
	{
		std::vector<std::tuple<std::reference_wrapper<const gvk::model_t>, std::vector<size_t>>> result;
		add_tuple_or_indices(result, args...);
		return result;
	}
	
	extern std::tuple<std::vector<glm::vec3>, std::vector<uint32_t>> get_vertices_and_indices(const std::vector<std::tuple<std::reference_wrapper<const gvk::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes);
	extern std::tuple<avk::buffer, avk::buffer> create_vertex_and_index_buffers(const std::vector<std::tuple<std::reference_wrapper<const gvk::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, vk::BufferUsageFlags aUsageFlags = {}, avk::sync aSyncHandler = avk::sync::wait_idle());
	extern std::vector<glm::vec3> get_normals(const std::vector<std::tuple<std::reference_wrapper<const gvk::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes);
	extern avk::buffer create_normals_buffer(const std::vector<std::tuple<std::reference_wrapper<const gvk::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, avk::sync aSyncHandler = avk::sync::wait_idle());
	extern std::vector<glm::vec3> get_tangents(const std::vector<std::tuple<std::reference_wrapper<const gvk::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes);
	extern avk::buffer create_tangents_buffer(const std::vector<std::tuple<std::reference_wrapper<const gvk::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, avk::sync aSyncHandler = avk::sync::wait_idle());
	extern std::vector<glm::vec3> get_bitangents(const std::vector<std::tuple<std::reference_wrapper<const gvk::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes);
	extern avk::buffer create_bitangents_buffer(const std::vector<std::tuple<std::reference_wrapper<const gvk::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, avk::sync aSyncHandler = avk::sync::wait_idle());
	extern std::vector<glm::vec4> get_colors(const std::vector<std::tuple<std::reference_wrapper<const gvk::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, int aColorsSet);
	extern avk::buffer create_colors_buffer(const std::vector<std::tuple<std::reference_wrapper<const gvk::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, int _ColorsSet = 0, avk::sync aSyncHandler = avk::sync::wait_idle());
	extern std::vector<glm::vec4> get_bone_weights(const std::vector<std::tuple<std::reference_wrapper<const model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes);
	extern avk::buffer create_bone_weights_buffer(const std::vector<std::tuple<std::reference_wrapper<const model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, avk::sync aSyncHandler = avk::sync::wait_idle());
	extern std::vector<glm::uvec4> get_bone_indices(const std::vector<std::tuple<std::reference_wrapper<const model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes);
	extern avk::buffer create_bone_indices_buffer(const std::vector<std::tuple<std::reference_wrapper<const model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, avk::sync aSyncHandler = avk::sync::wait_idle());
	extern std::vector<glm::vec2> get_2d_texture_coordinates(const std::vector<std::tuple<std::reference_wrapper<const gvk::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet);
	extern avk::buffer create_2d_texture_coordinates_buffer(const std::vector<std::tuple<std::reference_wrapper<const gvk::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet = 0, avk::sync aSyncHandler = avk::sync::wait_idle());
	extern std::vector<glm::vec3> get_3d_texture_coordinates(const std::vector<std::tuple<std::reference_wrapper<const gvk::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet);
	extern avk::buffer create_3d_texture_coordinates_buffer(const std::vector<std::tuple<std::reference_wrapper<const gvk::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet = 0, avk::sync aSyncHandler = avk::sync::wait_idle());

}
