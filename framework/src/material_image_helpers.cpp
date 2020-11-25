#include <gvk.hpp>

namespace gvk
{
	static auto select_format(int aPreferredNumberOfTextureComponents, bool aLoadHdr, bool aLoadSrgb)
	{
		std::optional<vk::Format> imFmt = {};

		if (!imFmt.has_value() && aLoadHdr) {
			//if (stbi_is_hdr(aPath.c_str())) {
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
			//}
		}

		if (!imFmt.has_value() && aLoadSrgb) {
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

		return imFmt;
	}


	static auto map_format_gli_to_vk(const gli::format gliFmt)
	{
		std::optional<vk::Format> imFmt = {};

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
			break;

		// uncompressed formats, TODO other values?
		case gli::format::FORMAT_RGBA8_UNORM_PACK8:
			imFmt = vk::Format::eR8G8B8A8Unorm;
			break;

		}

		return imFmt;
	}

	static int map_format_to_stbi_channels(vk::Format aFormat)
	{
		if (avk::is_4component_format(aFormat)) {
			return STBI_rgb_alpha;
		}
		else if (avk::is_3component_format(aFormat)) {
			return STBI_rgb;
		}
		else if (avk::is_2component_format(aFormat)) {
			return STBI_grey_alpha;
		}
		else {
			return STBI_grey;
		}
	}

	avk::image create_cubemap_from_file(const std::string& aPath, vk::Format aFormat, bool aFlip,
		avk::memory_usage aMemoryUsage, avk::image_usage aImageUsage, avk::sync aSyncHandler, std::optional<gli::texture> aAlreadyLoadedGliTexture)
	{
		std::vector<avk::buffer> stagingBuffers;
		int width = 0;
		int height = 0;

		// ============ Compressed formats (DDS) ==========
		// TODO find better way to determine which library to use for a given image file
		if (avk::is_block_compressed_format(aFormat) || aAlreadyLoadedGliTexture.has_value()) {
			if (!aAlreadyLoadedGliTexture.has_value()) {
				aAlreadyLoadedGliTexture = gli::load(aPath);
			}
			auto& gliTex = aAlreadyLoadedGliTexture.value();

			if (gliTex.target() != gli::TARGET_CUBE) {
				throw gvk::runtime_error(fmt::format("The image '{}' is not intended to be used as a cubemap image. Can't load it.", aPath));
			}

			width = gliTex.extent()[0];
			height = gliTex.extent()[1];

			/*
			auto& sb = stagingBuffers.emplace_back(context().create_buffer(
				AVK_STAGING_BUFFER_MEMORY_USAGE,
				vk::BufferUsageFlagBits::eTransferSrc,
				avk::generic_buffer_meta::create_from_size(gliTex.size())
			));
			sb->fill(gliTex.data(), 0, avk::sync::not_required());
			*/
		}

		// TODO other formats

		// image must have flag set to be used for cubemap
		assert((static_cast<int>(aImageUsage) & static_cast<int>(avk::image_usage::cube_compatible)) > 0);

		auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
		aSyncHandler.establish_barrier_before_the_operation(avk::pipeline_stage::transfer, avk::read_memory_access{ avk::memory_access::transfer_read_access });

		auto numLayers = 6; // a cubemap image in Vulkan must have six layers, one for each side of the cube
		auto img = context().create_image(width, height, aFormat, numLayers, aMemoryUsage, aImageUsage);
		auto finalTargetLayout = img->target_layout(); // save for later, because first, we need to transfer something into it

		// 1. Transition image layout to eTransferDstOptimal
		img->transition_to_layout(vk::ImageLayout::eTransferDstOptimal, avk::sync::auxiliary_with_barriers(aSyncHandler, {}, {})); // no need for additional sync
		// TODO: The original implementation transitioned into cgb::image_format(_Format) format here, not to eTransferDstOptimal => Does it still work? If so, eTransferDstOptimal is fine.

		// 2. Copy buffer to image
		//assert(stagingBuffers.size() == 1);
		//avk::copy_buffer_to_image(stagingBuffers.front(), img, avk::sync::auxiliary_with_barriers(aSyncHandler, {}, {}));  // There should be no need to make any memory available or visible, the transfer-execution dependency chain should be fine
																																				// TODO: Verify the above ^ comment
		// Are MIP-maps required?
		//if (img->config().mipLevels > 1u) {
			// TODO find better way to determine which library to use for a given image file
			if (avk::is_block_compressed_format(aFormat) || aAlreadyLoadedGliTexture.has_value()) {
				assert(aAlreadyLoadedGliTexture.has_value());
				// 1st level is contained in stagingBuffer
				// 
				// Now let's load further levels from the GliTexture and upload them directly into the sub-levels

				auto maxLevels = img->config().mipLevels;

				auto& gliTex = aAlreadyLoadedGliTexture.value();

				assert(maxLevels <= gliTex.levels());
				assert(gliTex.faces() == 6);

				// TODO: Do we have to account for gliTex.base_level() and gliTex.max_level()?
				for (uint32_t level = 0; level < maxLevels; ++level)
				{
					for (uint32_t face = 0; face < 6; ++face)
					{
#if _DEBUG
						{
							glm::tvec3<GLsizei> levelExtent(gliTex.extent(level));
							auto imgExtent = img->config().extent;
							auto levelDivisor = std::pow(2u, level);
							imgExtent.width = imgExtent.width > 1u ? imgExtent.width / levelDivisor : 1u;
							imgExtent.height = imgExtent.height > 1u ? imgExtent.height / levelDivisor : 1u;
							imgExtent.depth = imgExtent.depth > 1u ? imgExtent.depth / levelDivisor : 1u;
							assert(levelExtent.x == static_cast<int>(imgExtent.width));
							assert(levelExtent.y == static_cast<int>(imgExtent.height));
							assert(levelExtent.z == static_cast<int>(imgExtent.depth));
						}
#endif

						auto& sb = stagingBuffers.emplace_back(context().create_buffer(
							AVK_STAGING_BUFFER_MEMORY_USAGE,
							vk::BufferUsageFlagBits::eTransferSrc,
							avk::generic_buffer_meta::create_from_size(gliTex.size(level))
						));
						sb->fill(gliTex.data(0, face, level), 0, avk::sync::not_required());

						// Memory writes are not overlapping => no barriers should be fine.
						avk::copy_buffer_to_image_layer_mip_level(sb, img, face, level, {}, avk::sync::auxiliary_with_barriers(aSyncHandler, {}, {}));
					}
				}
			}
			else {
				// TODO load other image files here, load MIP-map level 0
				assert(false);

				// For uncompressed formats, create MIP-maps via BLIT:
				img->generate_mip_maps(avk::sync::auxiliary_with_barriers(aSyncHandler, {}, {}));
			}
		//}

		commandBuffer.set_custom_deleter([lOwnedStagingBuffers = std::move(stagingBuffers)](){});

		// 3. Transition image layout to its target layout and handle lifetime of things via sync
		img->transition_to_layout(finalTargetLayout, avk::sync::auxiliary_with_barriers(aSyncHandler, {}, {}));

		aSyncHandler.establish_barrier_after_the_operation(avk::pipeline_stage::transfer, avk::write_memory_access{ avk::memory_access::transfer_write_access });
		auto result = aSyncHandler.submit_and_sync();
		assert(!result.has_value());
		return img;
	}

	avk::image create_cubemap_from_file(const std::string& aPath, bool aLoadHdrIfPossible, bool aLoadSrgbIfApplicable, bool aFlip,
		int aPreferredNumberOfTextureComponents, avk::memory_usage aMemoryUsage, avk::image_usage aImageUsage, avk::sync aSyncHandler)
	{
		std::optional<vk::Format> imFmt = {};

		std::optional<gli::texture> gliTex = gli::load(aPath);
		if (!gliTex.value().empty()) {

			// TODO: warn if image can't be flipped?
			if (aFlip && (!gli::is_compressed(gliTex.value().format()) || gli::is_s3tc_compressed(gliTex.value().format()))) {
				gliTex = gli::flip(gliTex.value());
			}

			imFmt = map_format_gli_to_vk(gliTex.value().format());
		}
		else {
			gliTex.reset();
		}

		if (!imFmt.has_value()) {
			imFmt = select_format(aPreferredNumberOfTextureComponents, aLoadHdrIfPossible && stbi_is_hdr(aPath.c_str()), aLoadSrgbIfApplicable);
		}

		return create_cubemap_from_file(aPath, imFmt.value(), aFlip, aMemoryUsage, aImageUsage, std::move(aSyncHandler), std::move(gliTex));
	}

	avk::image create_cubemap_from_file(const std::vector<std::string>& aPaths, vk::Format aFormat, bool aFlip,
		avk::memory_usage aMemoryUsage, avk::image_usage aImageUsage, avk::sync aSyncHandler, std::vector<std::optional<gli::texture>> aAlreadyLoadedGliTextures)
	{
		std::vector<avk::buffer> stagingBuffers;
		int width = 0;
		int height = 0;

		for (auto i = 0; i < 6; ++i)
		{
			auto& aPath = aPaths[i];
			auto& aAlreadyLoadedGliTexture = aAlreadyLoadedGliTextures[i];

			int current_width = 0;
			int current_height = 0;

			// ============ Compressed formats (DDS) ==========
			// TODO find better way to determine which library to use for a given image file
			if (avk::is_block_compressed_format(aFormat) || aAlreadyLoadedGliTexture.has_value()) {
				if (!aAlreadyLoadedGliTexture.has_value()) {
					aAlreadyLoadedGliTexture = gli::load(aPath);
				}
				auto& gliTex = aAlreadyLoadedGliTexture.value();

				if (gliTex.target() != gli::TARGET_2D) {
					throw gvk::runtime_error(fmt::format("The image '{}' is not intended to be used as a 2D image. Can't load it.", aPath));
				}

				current_width = gliTex.extent()[0];
				current_height = gliTex.extent()[1];

				/*
				auto& sb = stagingBuffers.emplace_back(context().create_buffer(
					AVK_STAGING_BUFFER_MEMORY_USAGE,
					vk::BufferUsageFlagBits::eTransferSrc,
					avk::generic_buffer_meta::create_from_size(gliTex.size())
				));
				sb->fill(gliTex.data(), 0, avk::sync::not_required());
				*/
			}
			// TODO other formats

			if (width > 0 && height > 0) {
				assert(current_width == width && current_height == height);
			}
			else {
				width = current_width;
				height = current_height;
			}
		}

		// image must have flag set to be used for cubemap
		assert((static_cast<int>(aImageUsage) & static_cast<int>(avk::image_usage::cube_compatible)) > 0);

		auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
		aSyncHandler.establish_barrier_before_the_operation(avk::pipeline_stage::transfer, avk::read_memory_access{ avk::memory_access::transfer_read_access });

		auto numLayers = 6; // a cubemap image in Vulkan must have six layers, one for each side of the cube
		auto img = context().create_image(width, height, aFormat, numLayers, aMemoryUsage, aImageUsage);
		auto finalTargetLayout = img->target_layout(); // save for later, because first, we need to transfer something into it

		// 1. Transition image layout to eTransferDstOptimal
		img->transition_to_layout(vk::ImageLayout::eTransferDstOptimal, avk::sync::auxiliary_with_barriers(aSyncHandler, {}, {})); // no need for additional sync
		// TODO: The original implementation transitioned into cgb::image_format(_Format) format here, not to eTransferDstOptimal => Does it still work? If so, eTransferDstOptimal is fine.

		// 2. Copy buffer to image
		//assert(stagingBuffers.size() == 1);
		//avk::copy_buffer_to_image(stagingBuffers.front(), img, avk::sync::auxiliary_with_barriers(aSyncHandler, {}, {}));  // There should be no need to make any memory available or visible, the transfer-execution dependency chain should be fine
																																				// TODO: Verify the above ^ comment
		// Are MIP-maps required?
		//if (img->config().mipLevels > 1u) {
			// TODO find better way to determine which library to use for a given image file
		for (uint32_t face = 0; face < 6; ++face)
		{
			auto& aAlreadyLoadedGliTexture = aAlreadyLoadedGliTextures[face];

			if (avk::is_block_compressed_format(aFormat) || aAlreadyLoadedGliTexture.has_value()) {
				assert(aAlreadyLoadedGliTexture.has_value());
				// 1st level is contained in stagingBuffer
				// 
				// Now let's load further levels from the GliTexture and upload them directly into the sub-levels

				auto maxLevels = img->config().mipLevels;

				auto& gliTex = aAlreadyLoadedGliTexture.value();

				assert(maxLevels <= gliTex.levels());

				// TODO: Do we have to account for gliTex.base_level() and gliTex.max_level()?
				for (uint32_t level = 0; level < maxLevels; ++level)
				{
#if _DEBUG
					{
						glm::tvec3<GLsizei> levelExtent(gliTex.extent(level));
						auto imgExtent = img->config().extent;
						auto levelDivisor = std::pow(2u, level);
						imgExtent.width = imgExtent.width > 1u ? imgExtent.width / levelDivisor : 1u;
						imgExtent.height = imgExtent.height > 1u ? imgExtent.height / levelDivisor : 1u;
						imgExtent.depth = imgExtent.depth > 1u ? imgExtent.depth / levelDivisor : 1u;
						assert(levelExtent.x == static_cast<int>(imgExtent.width));
						assert(levelExtent.y == static_cast<int>(imgExtent.height));
						assert(levelExtent.z == static_cast<int>(imgExtent.depth));
					}
#endif

					auto& sb = stagingBuffers.emplace_back(context().create_buffer(
						AVK_STAGING_BUFFER_MEMORY_USAGE,
						vk::BufferUsageFlagBits::eTransferSrc,
						avk::generic_buffer_meta::create_from_size(gliTex.size(level))
					));
					sb->fill(gliTex.data(0, 0, level), 0, avk::sync::not_required());

					// Memory writes are not overlapping => no barriers should be fine.
					avk::copy_buffer_to_image_layer_mip_level(sb, img, face, level, {}, avk::sync::auxiliary_with_barriers(aSyncHandler, {}, {}));
				}
			}
			else {
			// TODO load other image files here, load MIP-map level 0
			assert(false);

			// For uncompressed formats, create MIP-maps via BLIT:
			img->generate_mip_maps(avk::sync::auxiliary_with_barriers(aSyncHandler, {}, {}));
			}
			//}
		}

		commandBuffer.set_custom_deleter([lOwnedStagingBuffers = std::move(stagingBuffers)](){});

		// 3. Transition image layout to its target layout and handle lifetime of things via sync
		img->transition_to_layout(finalTargetLayout, avk::sync::auxiliary_with_barriers(aSyncHandler, {}, {}));

		aSyncHandler.establish_barrier_after_the_operation(avk::pipeline_stage::transfer, avk::write_memory_access{ avk::memory_access::transfer_write_access });
		auto result = aSyncHandler.submit_and_sync();
		assert(!result.has_value());
		return img;
	}

	avk::image create_cubemap_from_file(const std::vector<std::string>& aPaths, bool aLoadHdrIfPossible, bool aLoadSrgbIfApplicable, bool aFlip,
		int aPreferredNumberOfTextureComponents, avk::memory_usage aMemoryUsage, avk::image_usage aImageUsage, avk::sync aSyncHandler)
	{
		assert(aPaths.size() == 6);

		std::vector< std::optional<gli::texture>> gliTexs(aPaths.size());

		std::optional<vk::Format> combined_imFmt = {};

		for (auto i = 0; i < 6; ++i)
		{
			std::optional<vk::Format> imFmt = {};

			auto& aPath = aPaths[i];
			auto& gliTex = gliTexs[i];

			gliTex = gli::load(aPath);
			if (!gliTex.value().empty()) {

				// TODO: warn if image can't be flipped?
				if (aFlip && (!gli::is_compressed(gliTex.value().format()) || gli::is_s3tc_compressed(gliTex.value().format()))) {
					gliTex = gli::flip(gliTex.value());
				}

				imFmt = map_format_gli_to_vk(gliTex.value().format());
			}
			else {
				gliTex.reset();
			}

			if (!imFmt.has_value()) {
				imFmt = select_format(aPreferredNumberOfTextureComponents, aLoadHdrIfPossible && stbi_is_hdr(aPath.c_str()), aLoadSrgbIfApplicable);
			}

			if (combined_imFmt.has_value())
				assert(imFmt == combined_imFmt);
			else
				combined_imFmt = imFmt;			
		}

		return create_cubemap_from_file(aPaths, combined_imFmt.value(), aFlip, aMemoryUsage, aImageUsage, std::move(aSyncHandler), std::move(gliTexs));

	}


	avk::image create_image_from_file(const std::string& aPath, vk::Format aFormat, bool aFlip, 
		avk::memory_usage aMemoryUsage, avk::image_usage aImageUsage, avk::sync aSyncHandler, std::optional<gli::texture> aAlreadyLoadedGliTexture)
	{
		std::vector<avk::buffer> stagingBuffers;
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

			width = gliTex.extent()[0];
			height = gliTex.extent()[1];

			auto& sb = stagingBuffers.emplace_back(context().create_buffer(
				AVK_STAGING_BUFFER_MEMORY_USAGE,
				vk::BufferUsageFlagBits::eTransferSrc,
				avk::generic_buffer_meta::create_from_size(gliTex.size())
			));
			sb->fill(gliTex.data(), 0, avk::sync::not_required());
		}
		// ============ RGB 8-bit formats ==========
		else if (avk::is_uint8_format(aFormat) || avk::is_int8_format(aFormat)) {

			stbi_set_flip_vertically_on_load(aFlip);
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

			auto& sb = stagingBuffers.emplace_back(context().create_buffer(
				AVK_STAGING_BUFFER_MEMORY_USAGE,
				vk::BufferUsageFlagBits::eTransferSrc,
				avk::generic_buffer_meta::create_from_size(imageSize)
			));
			sb->fill(pixels, 0, avk::sync::not_required());

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
			// TODO check if desired channels match channels in file
			size_t imageSize = static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(desiredColorChannels);

			if (!pixels) {
				throw gvk::runtime_error(fmt::format("Couldn't load image from '{}' using stbi_loadf", aPath));
			}

			auto& sb = stagingBuffers.emplace_back(context().create_buffer(
				AVK_STAGING_BUFFER_MEMORY_USAGE,
				vk::BufferUsageFlagBits::eTransferSrc,
				avk::generic_buffer_meta::create_from_size(imageSize)
			));
			sb->fill(pixels, 0, avk::sync::not_required());

			stbi_image_free(pixels);
		}
		else {
			throw gvk::runtime_error("No loader for the given image format implemented.");
		}

		auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
		aSyncHandler.establish_barrier_before_the_operation(avk::pipeline_stage::transfer, avk::read_memory_access{ avk::memory_access::transfer_read_access });

		auto img = context().create_image(width, height, aFormat, 1, aMemoryUsage, aImageUsage);
		auto finalTargetLayout = img->target_layout(); // save for later, because first, we need to transfer something into it

		// 1. Transition image layout to eTransferDstOptimal
		img->transition_to_layout(vk::ImageLayout::eTransferDstOptimal, avk::sync::auxiliary_with_barriers(aSyncHandler, {}, {})); // no need for additional sync
		// TODO: The original implementation transitioned into cgb::image_format(_Format) format here, not to eTransferDstOptimal => Does it still work? If so, eTransferDstOptimal is fine.

		// 2. Copy buffer to image
		assert(stagingBuffers.size() == 1);
		avk::copy_buffer_to_image(stagingBuffers.front(), img, {}, avk::sync::auxiliary_with_barriers(aSyncHandler, {}, {}));  // There should be no need to make any memory available or visible, the transfer-execution dependency chain should be fine
																																				// TODO: Verify the above ^ comment
		// Are MIP-maps required?
		// TODO: if number of mipmap levels is 0, a full mipmap pyramid should be created when loading the image,
		// see https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/#2.11 (usually not allowed for compressed formats)
		if (img->config().mipLevels > 1u) {
			if (avk::is_block_compressed_format(aFormat)) {
				assert(aAlreadyLoadedGliTexture.has_value());
				// 1st level is contained in stagingBuffer
				// 
				// Now let's load further levels from the GliTexture and upload them directly into the sub-levels

				auto& gliTex = aAlreadyLoadedGliTexture.value();
				// TODO: Do we have to account for gliTex.base_level() and gliTex.max_level()?
				for (uint32_t level = 1; level < gliTex.levels(); ++level)
				{
#if _DEBUG
					{
						glm::tvec3<GLsizei> levelExtent(gliTex.extent(level));
						auto imgExtent = img->config().extent;
						auto levelDivisor = std::pow(2u, level);
						imgExtent.width = imgExtent.width > 1u ? imgExtent.width / levelDivisor : 1u;
						imgExtent.height = imgExtent.height > 1u ? imgExtent.height / levelDivisor : 1u;
						imgExtent.depth = imgExtent.depth > 1u ? imgExtent.depth / levelDivisor : 1u;
						assert(levelExtent.x == static_cast<int>(imgExtent.width));
						assert(levelExtent.y == static_cast<int>(imgExtent.height));
						assert(levelExtent.z == static_cast<int>(imgExtent.depth));
					}
#endif

					auto& sb = stagingBuffers.emplace_back(context().create_buffer(
						AVK_STAGING_BUFFER_MEMORY_USAGE,
						vk::BufferUsageFlagBits::eTransferSrc,
						avk::generic_buffer_meta::create_from_size(gliTex.size(level))
					));
					sb->fill(gliTex.data(0, 0, level), 0, avk::sync::not_required());

					// Memory writes are not overlapping => no barriers should be fine.
					avk::copy_buffer_to_image_mip_level(sb, img, level, {}, avk::sync::auxiliary_with_barriers(aSyncHandler, {}, {}));
				}
			}
			else {
				// For uncompressed formats, create MIP-maps via BLIT:
				img->generate_mip_maps(avk::sync::auxiliary_with_barriers(aSyncHandler, {}, {}));
			}
		}

		commandBuffer.set_custom_deleter([lOwnedStagingBuffers = std::move(stagingBuffers)](){});

		// 3. Transition image layout to its target layout and handle lifetime of things via sync
		img->transition_to_layout(finalTargetLayout, avk::sync::auxiliary_with_barriers(aSyncHandler, {}, {}));

		aSyncHandler.establish_barrier_after_the_operation(avk::pipeline_stage::transfer, avk::write_memory_access{ avk::memory_access::transfer_write_access });
		auto result = aSyncHandler.submit_and_sync();
		assert(!result.has_value());
		return img;
	}

	avk::image create_image_from_file(const std::string& aPath, bool aLoadHdrIfPossible, bool aLoadSrgbIfApplicable, bool aFlip, 
		int aPreferredNumberOfTextureComponents, avk::memory_usage aMemoryUsage, avk::image_usage aImageUsage, avk::sync aSyncHandler)
	{
		std::optional<vk::Format> imFmt = {};

		std::optional<gli::texture> gliTex = gli::load(aPath);
		if (!gliTex.value().empty()) {

			// TODO: warn if image can't be flipped?
			if (aFlip && (!gli::is_compressed(gliTex.value().format()) || gli::is_s3tc_compressed(gliTex.value().format()))) {
				gliTex = gli::flip(gliTex.value());
			}

			imFmt = map_format_gli_to_vk(gliTex.value().format());
		}
		else {
			gliTex.reset();
		}

		if (!imFmt.has_value()) {
			imFmt = select_format(aPreferredNumberOfTextureComponents, aLoadHdrIfPossible && stbi_is_hdr(aPath.c_str()), aLoadSrgbIfApplicable);
		}

		/* Never throws
		if (!imFmt.has_value()) {
			throw gvk::runtime_error(fmt::format("Could not determine the image format of image '{}'", aPath));
		}
		*/

		return create_image_from_file(aPath, imFmt.value(), aFlip, aMemoryUsage, aImageUsage, std::move(aSyncHandler), std::move(gliTex));
	}

	std::tuple<std::vector<material_gpu_data>, std::vector<avk::image_sampler>> convert_for_gpu_usage(
		const std::vector<gvk::material_config>& aMaterialConfigs, 
		bool aLoadTexturesInSrgb,
		bool aFlipTextures,
		avk::image_usage aImageUsage,
		avk::filter_mode aTextureFilterMode, 
		avk::border_handling_mode aBorderHandlingMode,
		avk::sync aSyncHandler)
	{
		// These are the texture names loaded from file -> mapped to vector of usage-pointers
		std::unordered_map<std::string, std::vector<int*>> texNamesToUsages;
		// Textures contained in this array shall be loaded into an sRGB format
		std::set<std::string> srgbTextures;
		
		// However, if some textures are missing, provide 1x1 px textures in those spots
		std::vector<int*> whiteTexUsages;				// Provide a 1x1 px almost everywhere in those cases,
		std::vector<int*> straightUpNormalTexUsages;	// except for normal maps, provide a normal pointing straight up there.

		std::vector<material_gpu_data> gpuMaterial;
		gpuMaterial.reserve(aMaterialConfigs.size()); // important because of the pointers

		for (auto& mc : aMaterialConfigs) {
			auto& gm = gpuMaterial.emplace_back();
			gm.mDiffuseReflectivity			= mc.mDiffuseReflectivity		 ;
			gm.mAmbientReflectivity			= mc.mAmbientReflectivity		 ;
			gm.mSpecularReflectivity		= mc.mSpecularReflectivity		 ;
			gm.mEmissiveColor				= mc.mEmissiveColor				 ;
			gm.mTransparentColor			= mc.mTransparentColor			 ;
			gm.mReflectiveColor				= mc.mReflectiveColor			 ;
			gm.mAlbedo						= mc.mAlbedo					 ;
																			 
			gm.mOpacity						= mc.mOpacity					 ;
			gm.mBumpScaling					= mc.mBumpScaling				 ;
			gm.mShininess					= mc.mShininess					 ;
			gm.mShininessStrength			= mc.mShininessStrength			 ;
																			
			gm.mRefractionIndex				= mc.mRefractionIndex			 ;
			gm.mReflectivity				= mc.mReflectivity				 ;
			gm.mMetallic					= mc.mMetallic					 ;
			gm.mSmoothness					= mc.mSmoothness				 ;
																			 
			gm.mSheen						= mc.mSheen						 ;
			gm.mThickness					= mc.mThickness					 ;
			gm.mRoughness					= mc.mRoughness					 ;
			gm.mAnisotropy					= mc.mAnisotropy				 ;
																			 
			gm.mAnisotropyRotation			= mc.mAnisotropyRotation		 ;
			gm.mCustomData					= mc.mCustomData				 ;
																			 
			gm.mDiffuseTexIndex				= -1;	
			if (mc.mDiffuseTex.empty()) {
				whiteTexUsages.push_back(&gm.mDiffuseTexIndex);
			}
			else {
				auto path = avk::clean_up_path(mc.mDiffuseTex);
				texNamesToUsages[path].push_back(&gm.mDiffuseTexIndex);
				if (aLoadTexturesInSrgb) {
					srgbTextures.insert(path);
				}
			}

			gm.mSpecularTexIndex			= -1;							 
			if (mc.mSpecularTex.empty()) {
				whiteTexUsages.push_back(&gm.mSpecularTexIndex);
			}
			else {
				texNamesToUsages[avk::clean_up_path(mc.mSpecularTex)].push_back(&gm.mSpecularTexIndex);
			}

			gm.mAmbientTexIndex				= -1;							 
			if (mc.mAmbientTex.empty()) {
				whiteTexUsages.push_back(&gm.mAmbientTexIndex);
			}
			else {
				auto path = avk::clean_up_path(mc.mAmbientTex);
				texNamesToUsages[path].push_back(&gm.mAmbientTexIndex);
				if (aLoadTexturesInSrgb) {
					srgbTextures.insert(path);
				}
			}

			gm.mEmissiveTexIndex			= -1;							 
			if (mc.mEmissiveTex.empty()) {
				whiteTexUsages.push_back(&gm.mEmissiveTexIndex);
			}
			else {
				texNamesToUsages[avk::clean_up_path(mc.mEmissiveTex)].push_back(&gm.mEmissiveTexIndex);
			}

			gm.mHeightTexIndex				= -1;							 
			if (mc.mHeightTex.empty()) {
				whiteTexUsages.push_back(&gm.mHeightTexIndex);
			}
			else {
				texNamesToUsages[avk::clean_up_path(mc.mHeightTex)].push_back(&gm.mHeightTexIndex);
			}

			gm.mNormalsTexIndex				= -1;							 
			if (mc.mNormalsTex.empty()) {
				straightUpNormalTexUsages.push_back(&gm.mNormalsTexIndex);
			}
			else {
				texNamesToUsages[avk::clean_up_path(mc.mNormalsTex)].push_back(&gm.mNormalsTexIndex);
			}

			gm.mShininessTexIndex			= -1;							 
			if (mc.mShininessTex.empty()) {
				whiteTexUsages.push_back(&gm.mShininessTexIndex);
			}
			else {
				texNamesToUsages[avk::clean_up_path(mc.mShininessTex)].push_back(&gm.mShininessTexIndex);
			}

			gm.mOpacityTexIndex				= -1;							 
			if (mc.mOpacityTex.empty()) {
				whiteTexUsages.push_back(&gm.mOpacityTexIndex);
			}
			else {
				texNamesToUsages[avk::clean_up_path(mc.mOpacityTex)].push_back(&gm.mOpacityTexIndex);
			}

			gm.mDisplacementTexIndex		= -1;							 
			if (mc.mDisplacementTex.empty()) {
				whiteTexUsages.push_back(&gm.mDisplacementTexIndex);
			}
			else {
				texNamesToUsages[avk::clean_up_path(mc.mDisplacementTex)].push_back(&gm.mDisplacementTexIndex);
			}

			gm.mReflectionTexIndex			= -1;							 
			if (mc.mReflectionTex.empty()) {
				whiteTexUsages.push_back(&gm.mReflectionTexIndex);
			}
			else {
				texNamesToUsages[avk::clean_up_path(mc.mReflectionTex)].push_back(&gm.mReflectionTexIndex);
			}

			gm.mLightmapTexIndex			= -1;							 
			if (mc.mLightmapTex.empty()) {
				whiteTexUsages.push_back(&gm.mLightmapTexIndex);
			}
			else {
				texNamesToUsages[avk::clean_up_path(mc.mLightmapTex)].push_back(&gm.mLightmapTexIndex);
			}

			gm.mExtraTexIndex				= -1;							 
			if (mc.mExtraTex.empty()) {
				whiteTexUsages.push_back(&gm.mExtraTexIndex);
			}
			else {
				auto path = avk::clean_up_path(mc.mExtraTex);
				texNamesToUsages[path].push_back(&gm.mExtraTexIndex);
				if (aLoadTexturesInSrgb) {
					srgbTextures.insert(path);
				}
			}
																			 
			gm.mDiffuseTexOffsetTiling		= mc.mDiffuseTexOffsetTiling	 ;
			gm.mSpecularTexOffsetTiling		= mc.mSpecularTexOffsetTiling	 ;
			gm.mAmbientTexOffsetTiling		= mc.mAmbientTexOffsetTiling	 ;
			gm.mEmissiveTexOffsetTiling		= mc.mEmissiveTexOffsetTiling	 ;
			gm.mHeightTexOffsetTiling		= mc.mHeightTexOffsetTiling		 ;
			gm.mNormalsTexOffsetTiling		= mc.mNormalsTexOffsetTiling	 ;
			gm.mShininessTexOffsetTiling	= mc.mShininessTexOffsetTiling	 ;
			gm.mOpacityTexOffsetTiling		= mc.mOpacityTexOffsetTiling	 ;
			gm.mDisplacementTexOffsetTiling	= mc.mDisplacementTexOffsetTiling;
			gm.mReflectionTexOffsetTiling	= mc.mReflectionTexOffsetTiling	 ;
			gm.mLightmapTexOffsetTiling		= mc.mLightmapTexOffsetTiling	 ;
			gm.mExtraTexOffsetTiling		= mc.mExtraTexOffsetTiling		 ;
		}

		std::vector<avk::image_sampler> imageSamplers;
		const auto numSamplers = texNamesToUsages.size() + (whiteTexUsages.empty() ? 0 : 1) + (straightUpNormalTexUsages.empty() ? 0 : 1);
		imageSamplers.reserve(numSamplers);

		auto getSync = [numSamplers, &aSyncHandler, lSyncCount = size_t{0}] () mutable -> avk::sync {
			++lSyncCount;
			if (lSyncCount < numSamplers) {
				return avk::sync::auxiliary_with_barriers(aSyncHandler, avk::sync::steal_before_handler_on_demand, {}); // Invoke external sync exactly once (if there is something to sync)
			}
			assert (lSyncCount == numSamplers);
			return std::move(aSyncHandler); // For the last image, pass the main sync => this will also have the after-handler invoked.
		};

		// Create the white texture and assign its index to all usages
		if (!whiteTexUsages.empty()) {
			imageSamplers.push_back(
				context().create_image_sampler(
					owned(context().create_image_view(
						owned(create_1px_texture({ 255, 255, 255, 255 }, vk::Format::eR8G8B8A8Unorm, avk::memory_usage::device, aImageUsage, getSync()))
					)),
					owned(context().create_sampler(avk::filter_mode::nearest_neighbor, avk::border_handling_mode::repeat))
				)
			);
			int index = static_cast<int>(imageSamplers.size() - 1);
			for (auto* img : whiteTexUsages) {
				*img = index;
			}
		}

		// Create the normal texture, containing a normal pointing straight up, and assign to all usages
		if (!straightUpNormalTexUsages.empty()) {
			imageSamplers.push_back(
				context().create_image_sampler(
					owned(context().create_image_view(
						owned(create_1px_texture({ 127, 127, 255, 0 }, vk::Format::eR8G8B8A8Unorm, avk::memory_usage::device, aImageUsage, getSync()))
					)),
					owned(context().create_sampler(avk::filter_mode::nearest_neighbor, avk::border_handling_mode::repeat))
				)
			);
			int index = static_cast<int>(imageSamplers.size() - 1);
			for (auto* img : straightUpNormalTexUsages) {
				*img = index;
			}
		}

		// Load all the images from file, and assign them to all usages
		for (auto& pair : texNamesToUsages) {
			assert (!pair.first.empty());

			bool potentiallySrgb = srgbTextures.contains(pair.first);
			
			imageSamplers.push_back(
				context().create_image_sampler(
					owned(context().create_image_view(
						owned(create_image_from_file(pair.first, true, potentiallySrgb, aFlipTextures, 4, avk::memory_usage::device, aImageUsage, getSync()))
					)),
					owned(context().create_sampler(aTextureFilterMode, aBorderHandlingMode))
				)
			);
			int index = static_cast<int>(imageSamplers.size() - 1);
			for (auto* img : pair.second) {
				*img = index;
			}
		}

		// Hand over ownership to the caller
		return std::make_tuple(std::move(gpuMaterial), std::move(imageSamplers));
	}

	std::tuple<std::vector<glm::vec3>, std::vector<uint32_t>> get_vertices_and_indices(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes)
	{
		std::vector<glm::vec3> positionsData;
		std::vector<uint32_t> indicesData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<avk::resource_reference<const gvk::model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<size_t>>(pair)) {
				gvk::append_indices_and_vertex_data(
					gvk::additional_index_data(	indicesData,	[&]() { return modelRef.get().indices_for_mesh<uint32_t>(meshIndex);	} ),
					gvk::additional_vertex_data(positionsData,	[&]() { return modelRef.get().positions_for_mesh(meshIndex);			} )
				);
			}
		}

		return std::make_tuple( std::move(positionsData), std::move(indicesData) );
	}
	
	std::tuple<avk::buffer, avk::buffer> create_vertex_and_index_buffers(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, vk::BufferUsageFlags aUsageFlags, avk::sync aSyncHandler)
	{
		auto [positionsData, indicesData] = get_vertices_and_indices(aModelsAndSelectedMeshes);

		auto positionsBuffer = context().create_buffer(
			avk::memory_usage::device, aUsageFlags,
			avk::vertex_buffer_meta::create_from_data(positionsData)
				.describe_only_member(positionsData[0], avk::content_description::position)
		);
		positionsBuffer->fill(positionsData.data(), 0, avk::sync::auxiliary_with_barriers(aSyncHandler, avk::sync::steal_before_handler_on_demand, {}));
		// It is fine to let positionsData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		auto indexBuffer = context().create_buffer(
			avk::memory_usage::device, aUsageFlags,
			avk::index_buffer_meta::create_from_data(indicesData)
		);
		indexBuffer->fill(indicesData.data(), 0, std::move(aSyncHandler));
		// It is fine to let indicesData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		return std::make_tuple(std::move(positionsBuffer), std::move(indexBuffer));
	}

	std::vector<glm::vec3> get_normals(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes)
	{
		std::vector<glm::vec3> normalsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<avk::resource_reference<const gvk::model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<size_t>>(pair)) {
				insert_into(normalsData, modelRef.get().normals_for_mesh(meshIndex));
			}
		}

		return normalsData;
	}
	
	avk::buffer create_normals_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, avk::sync aSyncHandler)
	{
		auto normalsData = get_normals(aModelsAndSelectedMeshes);
		
		auto normalsBuffer = context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(normalsData)
		);
		normalsBuffer->fill(normalsData.data(), 0, std::move(aSyncHandler));
		// It is fine to let normalsData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.
		
		return normalsBuffer;
	}

	std::vector<glm::vec3> get_tangents(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes)
	{
		std::vector<glm::vec3> tangentsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<avk::resource_reference<const gvk::model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<size_t>>(pair)) {
				insert_into(tangentsData, modelRef.get().tangents_for_mesh(meshIndex));
			}
		}

		return tangentsData;
	}
	
	avk::buffer create_tangents_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, avk::sync aSyncHandler)
	{
		auto tangentsData = get_tangents(aModelsAndSelectedMeshes);
		
		auto tangentsBuffer = context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(tangentsData)
		);
		tangentsBuffer->fill(tangentsData.data(), 0, std::move(aSyncHandler));
		// It is fine to let tangentsData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		return tangentsBuffer;
	}

	std::vector<glm::vec3> get_bitangents(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes)
	{
		std::vector<glm::vec3> bitangentsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<avk::resource_reference<const gvk::model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<size_t>>(pair)) {
				insert_into(bitangentsData, modelRef.get().bitangents_for_mesh(meshIndex));
			}
		}

		return bitangentsData;
	}
	
	avk::buffer create_bitangents_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, avk::sync aSyncHandler)
	{
		auto bitangentsData = get_bitangents(aModelsAndSelectedMeshes);
		
		auto bitangentsBuffer = context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(bitangentsData)
		);
		bitangentsBuffer->fill(bitangentsData.data(), 0, std::move(aSyncHandler));
		// It is fine to let bitangentsData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		return bitangentsBuffer;
	}

	std::vector<glm::vec4> get_colors(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, int aColorsSet)
	{
		std::vector<glm::vec4> colorsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<avk::resource_reference<const gvk::model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<size_t>>(pair)) {
				insert_into(colorsData, modelRef.get().colors_for_mesh(meshIndex, aColorsSet));
			}
		}

		return colorsData;
	}
	
	avk::buffer create_colors_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, int aColorsSet, avk::sync aSyncHandler)
	{
		auto colorsData = get_colors(aModelsAndSelectedMeshes, aColorsSet);
		
		auto colorsBuffer = context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(colorsData)
		);
		colorsBuffer->fill(colorsData.data(), 0, std::move(aSyncHandler));
		// It is fine to let colorsData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		return colorsBuffer;
	}

	std::vector<glm::vec4> get_bone_weights(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes)
	{
		std::vector<glm::vec4> boneWeightsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<avk::resource_reference<const gvk::model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<size_t>>(pair)) {
				insert_into(boneWeightsData, modelRef.get().bone_weights_for_mesh(meshIndex));
			}
		}

		return boneWeightsData;
	}
	
	avk::buffer create_bone_weights_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, avk::sync aSyncHandler)
	{
		auto boneWeightsData = get_bone_weights(aModelsAndSelectedMeshes);
		
		auto boneWeightsBuffer = context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(boneWeightsData)
		);
		boneWeightsBuffer->fill(boneWeightsData.data(), 0, std::move(aSyncHandler));
		// It is fine to let boneWeightsData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		return boneWeightsBuffer;
	}

	std::vector<glm::uvec4> get_bone_indices(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes)
	{
		std::vector<glm::uvec4> boneIndicesData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<avk::resource_reference<const gvk::model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<size_t>>(pair)) {
				insert_into(boneIndicesData, modelRef.get().bone_indices_for_mesh(meshIndex));
			}
		}

		return boneIndicesData;
	}
	
	avk::buffer create_bone_indices_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, avk::sync aSyncHandler)
	{
		auto boneIndicesData = get_bone_indices(aModelsAndSelectedMeshes);
		
		auto boneIndicesBuffer = context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(boneIndicesData)
		);
		boneIndicesBuffer->fill(boneIndicesData.data(), 0, std::move(aSyncHandler));
		// It is fine to let boneIndicesData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		return boneIndicesBuffer;
	}

	std::vector<glm::vec2> get_2d_texture_coordinates(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet)
	{
		std::vector<glm::vec2> texCoordsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<avk::resource_reference<const gvk::model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<size_t>>(pair)) {
				insert_into(texCoordsData, modelRef.get().texture_coordinates_for_mesh<glm::vec2>(meshIndex, aTexCoordSet));
			}
		}

		return texCoordsData;
	}
	
	avk::buffer create_2d_texture_coordinates_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet, avk::sync aSyncHandler)
	{
		auto texCoordsData = get_2d_texture_coordinates(aModelsAndSelectedMeshes, aTexCoordSet);
		
		auto texCoordsBuffer = context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(texCoordsData)
		);
		texCoordsBuffer->fill(texCoordsData.data(), 0, std::move(aSyncHandler));
		// It is fine to let texCoordsData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		return texCoordsBuffer;
	}

	std::vector<glm::vec2> get_2d_texture_coordinates_flipped(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet)
	{
		std::vector<glm::vec2> texCoordsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<avk::resource_reference<const gvk::model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<size_t>>(pair)) {
				insert_into(texCoordsData, modelRef.get().texture_coordinates_for_mesh<glm::vec2>([](const glm::vec2& aValue){ return glm::vec2{aValue.x, 1.0f - aValue.y}; }, meshIndex, aTexCoordSet));
			}
		}

		return texCoordsData;
	}
	
	avk::buffer create_2d_texture_coordinates_flipped_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet, avk::sync aSyncHandler)
	{
		auto texCoordsData = get_2d_texture_coordinates_flipped(aModelsAndSelectedMeshes, aTexCoordSet);
		
		auto texCoordsBuffer = context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(texCoordsData)
		);
		texCoordsBuffer->fill(texCoordsData.data(), 0, std::move(aSyncHandler));
		// It is fine to let texCoordsData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		return texCoordsBuffer;
	}

	std::vector<glm::vec3> get_3d_texture_coordinates(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet)
	{
		std::vector<glm::vec3> texCoordsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<avk::resource_reference<const gvk::model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<size_t>>(pair)) {
				insert_into(texCoordsData, modelRef.get().texture_coordinates_for_mesh<glm::vec3>(meshIndex, aTexCoordSet));
			}
		}

		return texCoordsData;
	}
	
	avk::buffer create_3d_texture_coordinates_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet, avk::sync aSyncHandler)
	{
		auto texCoordsData = get_3d_texture_coordinates(aModelsAndSelectedMeshes, aTexCoordSet);
		
		auto texCoordsBuffer = context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(texCoordsData)
		);
		texCoordsBuffer->fill(texCoordsData.data(), 0, std::move(aSyncHandler));
		// It is fine to let texCoordsData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		return texCoordsBuffer;
	}

}