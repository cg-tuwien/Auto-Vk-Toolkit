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

	static avk::image create_image_from_file_cached(const std::string& aPath, vk::Format aFormat, bool aFlip = true, avk::memory_usage aMemoryUsage = avk::memory_usage::device, avk::image_usage aImageUsage = avk::image_usage::general_texture, avk::sync aSyncHandler = avk::sync::wait_idle(), std::optional<gli::texture> aAlreadyLoadedGliTexture = {}, std::optional<std::reference_wrapper<gvk::serializer>> aSerializer = {})
	{
		std::vector<avk::buffer> stagingBuffers;
		int width = 0;
		int height = 0;

		// ============ Compressed formats (DDS) ==========
		if (avk::is_block_compressed_format(aFormat)) {
			size_t texSize = 0;
			void* texData = nullptr;
			if (!aSerializer ||
				(aSerializer && aSerializer->get().mode() == gvk::serializer::mode::serialize)) {
				if (!aAlreadyLoadedGliTexture.has_value()) {
					aAlreadyLoadedGliTexture = gli::load(aPath);
				}
				auto& gliTex = aAlreadyLoadedGliTexture.value();

				if (gliTex.target() != gli::TARGET_2D) {
					throw gvk::runtime_error(fmt::format("The image '{}' is not intended to be used as 2D image. Can't load it.", aPath));
				}

				texSize = gliTex.size();
				texData = gliTex.data();

				width = gliTex.extent()[0];
				height = gliTex.extent()[1];
			}

			if (aSerializer) {
				aSerializer->get().archive(texSize);
				aSerializer->get().archive(width);
				aSerializer->get().archive(height);
			}

			auto& sb = stagingBuffers.emplace_back(context().create_buffer(
				AVK_STAGING_BUFFER_MEMORY_USAGE,
				vk::BufferUsageFlagBits::eTransferSrc,
				avk::generic_buffer_meta::create_from_size(texSize)
			));

			if (!aSerializer) {
				sb->fill(texData, 0, avk::sync::not_required());
			}
			else if (aSerializer && aSerializer->get().mode() == gvk::serializer::mode::serialize) {
				sb->fill(texData, 0, avk::sync::not_required());
				aSerializer->get().archive_memory(texData, texSize);
			}
			else if (aSerializer && aSerializer->get().mode() == gvk::serializer::mode::deserialize) {
				aSerializer->get().archive_buffer(sb);
			}
		}
		// ============ RGB 8-bit formats ==========
		else if (avk::is_uint8_format(aFormat) || avk::is_int8_format(aFormat)) {
			size_t imageSize = 0;
			stbi_uc* pixels = nullptr;
			if (!aSerializer ||
				(aSerializer && aSerializer->get().mode() == gvk::serializer::mode::serialize)) {
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
				pixels = stbi_load(aPath.c_str(), &width, &height, &channelsInFile, desiredColorChannels);
				imageSize = static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(desiredColorChannels);

				if (!pixels) {
					throw gvk::runtime_error(fmt::format("Couldn't load image from '{}' using stbi_load", aPath));
				}
			}

			if (aSerializer) {
				aSerializer->get().archive(imageSize);
				aSerializer->get().archive(width);
				aSerializer->get().archive(height);
			}

			auto& sb = stagingBuffers.emplace_back(context().create_buffer(
				AVK_STAGING_BUFFER_MEMORY_USAGE,
				vk::BufferUsageFlagBits::eTransferSrc,
				avk::generic_buffer_meta::create_from_size(imageSize)
			));

			if (!aSerializer) {
				sb->fill(pixels, 0, avk::sync::not_required());
			}
			else if (aSerializer && aSerializer->get().mode() == gvk::serializer::mode::serialize) {
				sb->fill(pixels, 0, avk::sync::not_required());
				aSerializer->get().archive_memory(pixels, imageSize);
			}
			else if (aSerializer && aSerializer->get().mode() == gvk::serializer::mode::deserialize) {
				aSerializer->get().archive_buffer(sb);
			}
		}
		// ============ RGB 16-bit float formats (HDR) ==========
		else if (avk::is_float16_format(aFormat)) {
			size_t imageSize = 0;
			float* pixels = nullptr;
			if (!aSerializer ||
				(aSerializer && aSerializer->get().mode() == gvk::serializer::mode::serialize)) {
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
				pixels = stbi_loadf(aPath.c_str(), &width, &height, &channelsInFile, desiredColorChannels);
				imageSize = static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(desiredColorChannels);

				if (!pixels) {
					throw gvk::runtime_error(fmt::format("Couldn't load image from '{}' using stbi_loadf", aPath));
				}
			}

			if (aSerializer) {
				aSerializer->get().archive(imageSize);
				aSerializer->get().archive(width);
				aSerializer->get().archive(height);
			}

			auto& sb = stagingBuffers.emplace_back(context().create_buffer(
				AVK_STAGING_BUFFER_MEMORY_USAGE,
				vk::BufferUsageFlagBits::eTransferSrc,
				avk::generic_buffer_meta::create_from_size(imageSize)
			));

			if (!aSerializer) {
				sb->fill(pixels, 0, avk::sync::not_required());
			}
			else if (aSerializer && aSerializer->get().mode() == gvk::serializer::mode::serialize) {
				sb->fill(pixels, 0, avk::sync::not_required());
				aSerializer->get().archive_memory(pixels, imageSize);
			}
			else if (aSerializer && aSerializer->get().mode() == gvk::serializer::mode::deserialize) {
				aSerializer->get().archive_buffer(sb);
			}
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
		assert(stagingBuffers.size() == 1);
		avk::copy_buffer_to_image(avk::const_referenced(stagingBuffers.front()), avk::referenced(img), {}, avk::sync::auxiliary_with_barriers(aSyncHandler, {}, {}));  // There should be no need to make any memory available or visible, the transfer-execution dependency chain should be fine
																																						// TODO: Verify the above ^ comment
		// Are MIP-maps required?
		if (img->config().mipLevels > 1u) {
			if (avk::is_block_compressed_format(aFormat)) {
				size_t levels = 0;
				if (!aSerializer ||
					(aSerializer && aSerializer->get().mode() == gvk::serializer::mode::serialize)) {
					assert(aAlreadyLoadedGliTexture.has_value());
					levels = aAlreadyLoadedGliTexture.value().levels();
				}
				if (aSerializer) {
					aSerializer->get().archive(levels);
				}
				// 1st level is contained in stagingBuffer
				//
				// Now let's load further levels from the GliTexture and upload them directly into the sub-levels

				// TODO: Do we have to account for gliTex.base_level() and gliTex.max_level()?
				for(size_t level = 1; level < levels; ++level)
				{
					size_t texSize = 0;
					void* texData = nullptr;
					glm::tvec3<GLsizei> levelExtent;

					if (!aSerializer ||
						(aSerializer && aSerializer->get().mode() == gvk::serializer::mode::serialize)) {
						texSize = aAlreadyLoadedGliTexture.value().size(level);
						texData = aAlreadyLoadedGliTexture.value().data(0, 0, level);
						auto& gliTex = aAlreadyLoadedGliTexture.value();
						levelExtent = gliTex.extent(level);
					}
					if (aSerializer) {
						aSerializer->get().archive(texSize);
						aSerializer->get().archive(levelExtent);
					}
#if _DEBUG
					{
						auto imgExtent = img->config().extent;
						auto levelDivisor = std::pow(2u, level);
						imgExtent.width  = imgExtent.width  > 1u ? imgExtent.width  / levelDivisor : 1u;
						imgExtent.height = imgExtent.height > 1u ? imgExtent.height / levelDivisor : 1u;
						imgExtent.depth  = imgExtent.depth  > 1u ? imgExtent.depth  / levelDivisor : 1u;
						assert (levelExtent.x == static_cast<int>(imgExtent.width ));
						assert (levelExtent.y == static_cast<int>(imgExtent.height));
						assert (levelExtent.z == static_cast<int>(imgExtent.depth ));
					}
#endif

					auto& sb = stagingBuffers.emplace_back(context().create_buffer(
						AVK_STAGING_BUFFER_MEMORY_USAGE,
						vk::BufferUsageFlagBits::eTransferSrc,
						avk::generic_buffer_meta::create_from_size(texSize)
					));

					if (!aSerializer) {
						sb->fill(texData, 0, avk::sync::not_required());
					}
					else if (aSerializer && aSerializer->get().mode() == gvk::serializer::mode::serialize) {
						sb->fill(texData, 0, avk::sync::not_required());
						aSerializer->get().archive_memory(texData, texSize);
					}
					else if (aSerializer && aSerializer->get().mode() == gvk::serializer::mode::deserialize) {
						aSerializer->get().archive_buffer(sb);
					}

					// Memory writes are not overlapping => no barriers should be fine.
					avk::copy_buffer_to_image_mip_level(avk::const_referenced(sb), avk::referenced(img), level, {}, avk::sync::auxiliary_with_barriers(aSyncHandler, {}, {}));
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

	static avk::image create_image_from_file_cached(gvk::serializer& aSerializer,const std::string& aPath, vk::Format aFormat, bool aFlip = true, avk::memory_usage aMemoryUsage = avk::memory_usage::device, avk::image_usage aImageUsage = avk::image_usage::general_texture, avk::sync aSyncHandler = avk::sync::wait_idle(), std::optional<gli::texture> aAlreadyLoadedGliTexture = {})
	{
		return create_image_from_file_cached(aPath, aFormat, aFlip, aMemoryUsage, aImageUsage, std::move(aSyncHandler), std::move(aAlreadyLoadedGliTexture), aSerializer);
	}

	static avk::image create_image_from_file(const std::string& aPath, vk::Format aFormat, bool aFlip = true, avk::memory_usage aMemoryUsage = avk::memory_usage::device, avk::image_usage aImageUsage = avk::image_usage::general_texture, avk::sync aSyncHandler = avk::sync::wait_idle(), std::optional<gli::texture> aAlreadyLoadedGliTexture = {})
	{
		return create_image_from_file_cached(aPath, aFormat, aFlip, aMemoryUsage, aImageUsage, std::move(aSyncHandler), std::move(aAlreadyLoadedGliTexture));
	}

	static avk::image create_image_from_file_cached(const std::string& aPath, bool aLoadHdrIfPossible = true, bool aLoadSrgbIfApplicable = true, bool aFlip = true, int aPreferredNumberOfTextureComponents = 4, avk::memory_usage aMemoryUsage = avk::memory_usage::device, avk::image_usage aImageUsage = avk::image_usage::general_texture, avk::sync aSyncHandler = avk::sync::wait_idle(), std::optional<std::reference_wrapper<gvk::serializer>> aSerializer = {})
	{
		std::optional<vk::Format> imFmt = {};

		std::optional<gli::texture> gliTex = {};
		if (!aSerializer ||
			(aSerializer && aSerializer->get().mode() == gvk::serializer::mode::serialize)) {
			gliTex = gli::load(aPath);
			if (!gliTex.value().empty()) {

				if (aFlip && (!gli::is_compressed(gliTex.value().format()) || gli::is_s3tc_compressed(gliTex.value().format()))) {
					gliTex = gli::flip(gliTex.value());
				}

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

		}

		if (aSerializer) {
			aSerializer->get().archive(imFmt);
		}

		if (!imFmt.has_value()) {
			throw gvk::runtime_error(fmt::format("Could not determine the image format of image '{}'", aPath));
		}

		return create_image_from_file_cached(aPath, imFmt.value(), aFlip, aMemoryUsage, aImageUsage, std::move(aSyncHandler), std::move(gliTex), aSerializer);
	}

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

	/**	This is a convenience method that allows to compile a selection of models and mesh indices.
	 *	Valid usage means passing a model as parameter, and following it up with one or multiple mesh index parameters.
	 *
	 *	Model parameters must be bindable by const model&.
	 *	Mesh index parameters are supported in the forms:
	 *	 - size_t
	 *	 - std::vector<size_t>
	 *
	 *	Examples of valid parameters:
	 *	 - make_models_and_meshes_selection(myModel, 1, 2, 3); // => a model and 3x size_t
	 *	 - make_models_and_meshes_selection(myModel, 1, std::vector<size_t>{ 2, 3 }); // => a model and then 1x size_t, and 1x vector of size_t (containing two indices)
	 *	 - make_models_and_meshes_selection(myModel, 1, myOtherModel, std::vector<size_t>{ 0, 1 }); // => a model and then 1x size_t; another model and 1x vector of size_t (containing two indices)
	 */
	template <typename... Args>
	std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>> make_models_and_meshes_selection(const Args&... args)
	{
		std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>> result;
		add_tuple_or_indices(result, args...);
		return result;
	}

	/** Helper function to fill a given device buffer with a staging buffer whose contents are loaded from a cache file.
	 *	@param	aSerializer		The serializer in gvk::serializer::mode::deserialize
	 *	@param	aDevicebuffer	The target buffer
	 *	@param	aTotalSize		Size of the staging buffer
	 *	@para	aSyncHandler	Synchronization handler
	 */
	static inline void fill_device_buffer_from_cache(gvk::serializer& aSerializer, avk::buffer& aDeviceBuffer, size_t aTotalSize, avk::sync& aSyncHandler)
	{
		assert(aSerializer.mode() == gvk::serializer::mode::deserialize);
		
		// Create host visible staging buffer for filling on host side from file
		auto sb = context().create_buffer(
			AVK_STAGING_BUFFER_MEMORY_USAGE,
			vk::BufferUsageFlagBits::eTransferSrc,
			avk::generic_buffer_meta::create_from_size(aTotalSize)
		);
		// Let the serializer map and fill the buffer
		aSerializer.archive_buffer(sb);

		auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
		// Sync before
		aSyncHandler.establish_barrier_before_the_operation(avk::pipeline_stage::transfer, avk::read_memory_access{ avk::memory_access::transfer_read_access });

		// Copy host visible staging buffer to device buffer
		avk::copy_buffer_to_another(avk::referenced(sb), avk::referenced(aDeviceBuffer), 0, 0, aTotalSize, avk::sync::with_barriers_into_existing_command_buffer(commandBuffer, {}, {}));

		// Sync after
		aSyncHandler.establish_barrier_after_the_operation(avk::pipeline_stage::transfer, avk::write_memory_access{ avk::memory_access::transfer_write_access });

		// Take care of the lifetime handling of the stagingBuffer, it might still be in use
		commandBuffer.set_custom_deleter([
			lOwnedStagingBuffer{ std::move(sb) }
		]() { /* Nothing to do here, the buffers' destructors will do the cleanup, the lambda is just storing it. */ });

		// Finish him
		aSyncHandler.submit_and_sync();
	}

	/**	Get a tuple of <0>:vertices and <1>:indices from the given selection of models and associated mesh indices.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@return	Combined position and index data of all specified model + mesh-indices tuples, where the returned tuple's elements refer to:
	 *			<0>: vertex positions
	 *			<1>: indices
	 */
	extern std::tuple<std::vector<glm::vec3>, std::vector<uint32_t>> get_vertices_and_indices(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes);

	/**	Get a tuple of <0>:vertices and <1>:indices from the given selection of models and associated mesh indices.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. The order is maintained.
	 *	@return	Combined position and index data of all specified model + mesh-indices tuples, where the returned tuple's elements refer to:
	 *			<0>: vertex positions
	 *			<1>: indices
	 */
	extern std::tuple<std::vector<glm::vec3>, std::vector<uint32_t>> get_vertices_and_indices_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes);

	// A concept which requires a type to have a .set_format<glm::uvec3>(avk::content_description)
	template <typename T>
	concept has_set_format_for_index_buffer = requires (T x)
	{
		x.template set_format<glm::uvec3>(avk::content_description::index);
	};
	
	// Helper which creates meta data for uniform/storage_texel_buffer_view metas:
	template <typename Meta>
	auto set_up_meta_for_index_buffer(const std::vector<uint32_t>& aIndicesData) requires has_set_format_for_index_buffer<Meta>
	{
		return Meta::create_from_data(aIndicesData).template set_format<glm::uvec3>(avk::content_description::index); // Combine 3 consecutive elements to one unit
	}

	// ...and another helper which creates metas for other Meta types:
	template <typename Meta>
	auto set_up_meta_for_index_buffer(const std::vector<uint32_t>& aIndicesData)
	{
		return Meta::create_from_data(aIndicesData).describe_only_member(aIndicesData[0], avk::content_description::index);
	}

	/**	Get a tuple of two buffers, containing vertex positions and index positions, respectively, from the given input data.
	 *	@param	aVerticesAndIndices			A tuple containing vertex positions data in the first element, and index data in the second element.
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@param	aSyncHandler				A synchronization handler.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of each buffer besides their obligatory
	 *										avk::vertex_buffer_meta, and avk::index_buffer_meta, as appropriate for the two buffers.
	 *										The additional meta data declarations will always refer to the whole data in the buffers; specifying subranges is not supported.
	 *	@return	A tuple of two buffers in device memory which contain the given input data, where the returned tuple's elements refer to:
	 *			<0>: buffer containing vertex positions
	 *			<1>: buffer containing indices
	 */
	template <typename... Metas>
	std::tuple<avk::buffer, avk::buffer> create_vertex_and_index_buffers(const std::tuple<std::vector<glm::vec3>, std::vector<uint32_t>>& aVerticesAndIndices, vk::BufferUsageFlags aUsageFlags = {}, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();

		// Sync before: 
		// TODO: This is actually not necessary, because the command submission makes the data available => remove this barrier, actually?!?!!
		aSyncHandler.establish_barrier_before_the_operation(avk::pipeline_stage::transfer, avk::read_memory_access{ avk::memory_access::transfer_read_access });

		const auto& [positionsData, indicesData] = aVerticesAndIndices;

		auto positionsBuffer = context().create_buffer(
			avk::memory_usage::device, aUsageFlags,
			avk::vertex_buffer_meta::create_from_data(positionsData).describe_only_member(positionsData[0], avk::content_description::position),
			Metas::create_from_data(positionsData).describe_only_member(positionsData[0], avk::content_description::position)...
		);
		positionsBuffer->fill(positionsData.data(), 0, avk::sync::auxiliary_with_barriers(aSyncHandler, {}, {}));
		// It is fine to let positionsData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		auto indexBuffer = context().create_buffer(
			avk::memory_usage::device, aUsageFlags,
			avk::index_buffer_meta::create_from_data(indicesData),
			set_up_meta_for_index_buffer<Metas>(indicesData)...
		);
		indexBuffer->fill(indicesData.data(), 0, avk::sync::auxiliary_with_barriers(aSyncHandler, {}, {}));
		// It is fine to let indicesData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		// Sync after:
		aSyncHandler.establish_barrier_after_the_operation(avk::pipeline_stage::transfer, avk::write_memory_access{ avk::memory_access::transfer_write_access });

		// Finish him:
		aSyncHandler.submit_and_sync(); // Return command buffer is not supported here.

		return std::make_tuple(std::move(positionsBuffer), std::move(indexBuffer));
	}
	
	/**	Get a tuple of two buffers, containing vertex positions and index positions, respectively, from the given input data.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@param	aSyncHandler				A synchronization handler.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of each buffer besides their obligatory
	 *										avk::vertex_buffer_meta, and avk::index_buffer_meta, as appropriate for the two buffers.
	 *										The additional meta data declarations will always refer to the whole data in the buffers; specifying subranges is not supported.
	 *	@return	A tuple of two buffers in device memory which contain the given input data, where the returned tuple's elements refer to:
	 *			<0>: buffer containing vertex positions
	 *			<1>: buffer containing indices
	 */
	template <typename... Metas>
	std::tuple<avk::buffer, avk::buffer> create_vertex_and_index_buffers(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, vk::BufferUsageFlags aUsageFlags = {}, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		return create_vertex_and_index_buffers<Metas...>(get_vertices_and_indices(aModelsAndSelectedMeshes), aUsageFlags, std::move(aSyncHandler));
	}
	
	/**	Get a tuple of two buffers, containing vertex positions and index positions, respectively, from the given input data.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aVerticesAndIndices			A tuple containing vertex positions data in the first element, and index data in the second element.
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@param	aSyncHandler				A synchronization handler.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of each buffer besides their obligatory
	 *										avk::vertex_buffer_meta, and avk::index_buffer_meta, as appropriate for the two buffers.
	 *										The additional meta data declarations will always refer to the whole data in the buffers; specifying subranges is not supported.
	 *	@return	A tuple of two buffers in device memory which contain the given input data, where the tuple elements refer to:
	 *			<0>: buffer containing vertex positions
	 *			<1>: buffer containing indices
	 */
	template <typename... Metas>
	std::tuple<avk::buffer, avk::buffer> create_vertex_and_index_buffers_cached(gvk::serializer& aSerializer, std::tuple<std::vector<glm::vec3>, std::vector<uint32_t>>& aVerticesAndIndices, vk::BufferUsageFlags aUsageFlags = {}, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		size_t numPositions = 0;
		size_t totalPositionsSize = 0;
		size_t numIndices = 0;
		size_t totalIndicesSize = 0;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			auto& [positionsData, indicesData] = aVerticesAndIndices;

			numPositions = positionsData.size();
			totalPositionsSize = sizeof(positionsData[0]) * numPositions;
			numIndices = indicesData.size();
			totalIndicesSize = sizeof(indicesData[0]) * numIndices;

			aSerializer.archive(numPositions);
			aSerializer.archive(totalPositionsSize);
			aSerializer.archive(numIndices);
			aSerializer.archive(totalIndicesSize);

			aSerializer.archive_memory(positionsData.data(), totalPositionsSize);
			aSerializer.archive_memory(indicesData.data(), totalIndicesSize);

			return create_vertex_and_index_buffers<Metas...>(std::make_tuple(std::move(positionsData), std::move(indicesData)), aUsageFlags, std::move(aSyncHandler));
		}
		else {
			aSerializer.archive(numPositions);
			aSerializer.archive(totalPositionsSize);
			aSerializer.archive(numIndices);
			aSerializer.archive(totalIndicesSize);

			auto positionsBuffer = context().create_buffer(
				avk::memory_usage::device, aUsageFlags,
				avk::vertex_buffer_meta::create_from_total_size(totalPositionsSize, numPositions)
				.describe_member(0, avk::format_for<glm::vec3>(), avk::content_description::position)
			);

			fill_device_buffer_from_cache(aSerializer, positionsBuffer, totalPositionsSize, aSyncHandler);

			auto indexBuffer = context().create_buffer(
				avk::memory_usage::device, aUsageFlags,
				avk::index_buffer_meta::create_from_total_size(totalIndicesSize, numIndices)
			);

			fill_device_buffer_from_cache(aSerializer, indexBuffer, totalIndicesSize, aSyncHandler);

			return std::make_tuple(std::move(positionsBuffer), std::move(indexBuffer));
		}
	}

	/**	Get a tuple of two buffers, containing vertex positions and index positions, respectively, from the given input data.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@param	aSyncHandler				A synchronization handler.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of each buffer besides their obligatory
	 *										avk::vertex_buffer_meta, and avk::index_buffer_meta, as appropriate for the two buffers.
	 *										The additional meta data declarations will always refer to the whole data in the buffers; specifying subranges is not supported.
	 *	@return	A tuple of two buffers in device memory which contain the given input data, where the tuple elements refer to:
	 *			<0>: buffer containing vertex positions
	 *			<1>: buffer containing indices
	 */
	template <typename... Metas>
	std::tuple<avk::buffer, avk::buffer> create_vertex_and_index_buffers_cached(gvk::serializer& aSerializer, std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, vk::BufferUsageFlags aUsageFlags = {}, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		std::tuple<std::vector<glm::vec3>, std::vector<uint32_t>> verticesAndIndicesData;
		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			verticesAndIndicesData = get_vertices_and_indices(aModelsAndSelectedMeshes);
		}
		return create_vertex_and_index_buffers_cached<Metas...>(aSerializer, verticesAndIndicesData, aUsageFlags, std::move(aSyncHandler));
	}

	/**	Create a device buffer that contains the given input data
	 *	@param	aBufferData					Data to be stored in the buffer
	 *	@param	aContentDescription			Description of the buffer's content
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@param	aSyncHandler				A synchronization handler.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A buffer in device memory which contains the given input data.
	 */
	template <typename T, typename... Metas>
	avk::buffer create_buffer(const T& aBufferData, avk::content_description aContentDescription, vk::BufferUsageFlags aUsageFlags = {}, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		auto buffer = context().create_buffer(
			avk::memory_usage::device, aUsageFlags,
			avk::generic_buffer_meta::create_from_data(aBufferData),
			Metas::create_from_data(aBufferData).describe_only_member(aBufferData[0], aContentDescription)...
		);
		buffer->fill(aBufferData.data(), 0, std::move(aSyncHandler));
		// It is fine to let aBufferData go out of scope, since its data has been copied to a
		// staging buffer within create_buffer, which is lifetime-handled by the command buffer.

		return buffer;
	}

	/** Create a device buffer that contains the given input data
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aBufferData					Data to be stored in the buffer
	 *	@param	aContentDescription			Description of the buffer's content
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@param	aSyncHandler				A synchronization handler.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A buffer in device memory which contains the given input data.
	 */
	template <typename T, typename... Metas>
	avk::buffer create_buffer_cached(gvk::serializer& aSerializer, T& aBufferData, avk::content_description aContentDescription, vk::BufferUsageFlags aUsageFlags = {}, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		size_t numBufferEntries = 0;
		size_t bufferTotalSize = 0;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			numBufferEntries = aBufferData.size();
			bufferTotalSize = sizeof(aBufferData[0]) * numBufferEntries;

			aSerializer.archive(numBufferEntries);
			aSerializer.archive(bufferTotalSize);

			aSerializer.archive_memory(aBufferData.data(), bufferTotalSize);

			return create_buffer<T, Metas...>(aBufferData, aContentDescription, aUsageFlags, std::move(aSyncHandler));
		}
		else {
			aSerializer.archive(numBufferEntries);
			aSerializer.archive(bufferTotalSize);

			auto buffer = context().create_buffer(
				avk::memory_usage::device, aUsageFlags,
				avk::vertex_buffer_meta::create_from_total_size(bufferTotalSize, numBufferEntries)
			);

			fill_device_buffer_from_cache(aSerializer, buffer, bufferTotalSize, aSyncHandler);

			return buffer;
		}
	}
	
	/**	Get normals from the given selection of models and associated mesh indices.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@return	Combined normals data of all specified model + mesh-indices.
	 */
	extern std::vector<glm::vec3> get_normals(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes);

	/**	Get normals from the given selection of models and associated mesh indices.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. The order is maintained.
	 *	@return	Combined normals data of all specified model + mesh-indices tuples.
	 */
	extern std::vector<glm::vec3> get_normals_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes);

	/**	Get a buffer containing normals from the given input data.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffer is created with.
	 *	@param	aSyncHandler				A synchronization handler.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A buffer in device memory which contains the given input data.
	 */
	template <typename... Metas>
	avk::buffer create_normals_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, vk::BufferUsageFlags aUsageFlags = {}, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		return create_buffer<std::vector<glm::vec3>, Metas...>(get_normals(aModelsAndSelectedMeshes), avk::content_description::normal, aUsageFlags, std::move(aSyncHandler));
	}

	/** Get a buffer containing normals from the given input data.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@param	aSyncHandler				A synchronization handler.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A buffer in device memory which contains the given input data.
	 */
	template <typename... Metas>
	avk::buffer create_normals_buffer_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, vk::BufferUsageFlags aUsageFlags = {}, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		std::vector<glm::vec3> normalsData;
		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			normalsData = get_normals(aModelsAndSelectedMeshes);
		}
		return create_buffer_cached<std::vector<glm::vec3>, Metas...>(aSerializer, normalsData, avk::content_description::normal, aUsageFlags, std::move(aSyncHandler));
	}

	/**	Get tangents from the given selection of models and associated mesh indices.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@return	Combined tangents data of all specified model + mesh-indices.
	 */
	extern std::vector<glm::vec3> get_tangents(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes);

	/**	Get tangents from the given selection of models and associated mesh indices.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. The order is maintained.
	 *	@return	Combined tangents data of all specified model + mesh-indices tuples.
	 */
	extern std::vector<glm::vec3> get_tangents_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes);
	
	/**	Get a buffer containing tangents from the given input data.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffer is created with.
	 *	@param	aSyncHandler				A synchronization handler.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A buffer in device memory which contains the given input data.
	 */
	template <typename... Metas>
	avk::buffer create_tangents_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, vk::BufferUsageFlags aUsageFlags = {}, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		return create_buffer<std::vector<glm::vec3>, Metas...>(get_tangents(aModelsAndSelectedMeshes), avk::content_description::tangent, aUsageFlags, std::move(aSyncHandler));
	}

	/** Get a buffer containing tangents from the given input data.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@param	aSyncHandler				A synchronization handler.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A buffer in device memory which contains the given input data.
	 */
	template <typename... Metas>
	avk::buffer create_tangents_buffer_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, vk::BufferUsageFlags aUsageFlags = {}, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		std::vector<glm::vec3> tangentsData;
		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			tangentsData = get_tangents(aModelsAndSelectedMeshes);
		}
		return create_buffer_cached<std::vector<glm::vec3>, Metas...>(aSerializer, tangentsData, avk::content_description::tangent, aUsageFlags, std::move(aSyncHandler));
	}
	
	/**	Get bitangents from the given selection of models and associated mesh indices.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@return	Combined bitangents data of all specified model + mesh-indices.
	 */
	extern std::vector<glm::vec3> get_bitangents(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes);

	/**	Get bitangents from the given selection of models and associated mesh indices.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. The order is maintained.
	 *	@return	Combined bitangents data of all specified model + mesh-indices tuples.
	 */
	extern std::vector<glm::vec3> get_bitangents_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes);

	/**	Get a buffer containing bitangents from the given input data.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffer is created with.
	 *	@param	aSyncHandler				A synchronization handler.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A buffer in device memory which contains the given input data.
	 */
	template <typename... Metas>
	avk::buffer create_bitangents_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, vk::BufferUsageFlags aUsageFlags = {}, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		return create_buffer<std::vector<glm::vec3>, Metas...>(get_bitangents(aModelsAndSelectedMeshes), avk::content_description::bitangent, aUsageFlags, std::move(aSyncHandler));
	}

	/** Get a buffer containing bitangents from the given input data.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@param	aSyncHandler				A synchronization handler.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A buffer in device memory which contains the given input data.
	 */
	template <typename... Metas>
	avk::buffer create_bitangents_buffer_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, vk::BufferUsageFlags aUsageFlags = {}, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		std::vector<glm::vec3> bitangentsData;
		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			bitangentsData = get_bitangents(aModelsAndSelectedMeshes);
		}
		return create_buffer_cached<std::vector<glm::vec3>, Metas...>(aSerializer, bitangentsData, avk::content_description::bitangent, aUsageFlags, std::move(aSyncHandler));
	}

	/**	Get colors from the given selection of models and associated mesh indices.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aColorsSet					The zero-based set of vertex colors to get
	 *	@return	Combined colors data of all specified model + mesh-indices.
	 */
	extern std::vector<glm::vec4> get_colors(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aColorsSet = 0);

	/**	Get colors from the given selection of models and associated mesh indices.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. The order is maintained.
	 *	@param	aColorsSet					The zero-based set of vertex colors to get
	 *	@return	Combined colors data of all specified model + mesh-indices tuples.
	 */
	extern std::vector<glm::vec4> get_colors_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aColorsSet = 0);
	
	/**	Get a buffer containing colors from the given input data.
		 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
		 *										All the data they refer to is combined into a a common result. Their order is maintained.
		 *	@param	aUsageFlags					Additional usage flags that the buffer is created with.
		 *	@param	aSyncHandler				A synchronization handler.
		 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
		 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
		 *	@return	A buffer in device memory which contains the given input data.
		 */
	template <typename... Metas>
	avk::buffer create_colors_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aColorsSet = 0, vk::BufferUsageFlags aUsageFlags = {}, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		return create_buffer<std::vector<glm::vec4>, Metas...>(get_colors(aModelsAndSelectedMeshes, aColorsSet), avk::content_description::color, aUsageFlags, std::move(aSyncHandler));
	}

	/** Get a buffer containing colors from the given input data.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@param	aSyncHandler				A synchronization handler.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A buffer in device memory which contains the given input data.
	 */
	template <typename... Metas>
	avk::buffer create_colors_buffer_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aColorsSet = 0, vk::BufferUsageFlags aUsageFlags = {}, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		std::vector<glm::vec4> colorsData;
		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			colorsData = get_colors(aModelsAndSelectedMeshes, aColorsSet);
		}
		return create_buffer_cached<std::vector<glm::vec4>, Metas...>(aSerializer, colorsData, avk::content_description::color, aUsageFlags, std::move(aSyncHandler));
	}

	/**	Get bone weights from the given selection of models and associated mesh indices.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aNormalizeBoneWeights		Set to true to apply normalization to the bone weights, s.t. their sum equals 1.
	 *	@return	Combined normals data of all specified model + mesh-indices.
	 */
	extern std::vector<glm::vec4> get_bone_weights(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, bool aNormalizeBoneWeights = false);

	/**	Get bone weights from the given selection of models and associated mesh indices.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. The order is maintained.
	 *	@param	aNormalizeBoneWeights		Set to true to apply normalization to the bone weights, s.t. their sum equals 1.
	 *	@return	Combined normals data of all specified model + mesh-indices tuples.
	 */
	extern std::vector<glm::vec4> get_bone_weights_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, bool aNormalizeBoneWeights = false);

	/**	Get a buffer containing bone weights from the given input data.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffer is created with.
	 *	@param	aSyncHandler				A synchronization handler.
	 *	@param	aNormalizeBoneWeights		Set to true to apply normalization to the bone weights, s.t. their sum equals 1.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A buffer in device memory which contains the given input data.
	 */
	template <typename... Metas>
	avk::buffer create_bone_weights_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, bool aNormalizeBoneWeights = false, vk::BufferUsageFlags aUsageFlags = {}, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		return create_buffer<std::vector<glm::vec4>, Metas...>(get_bone_weights(aModelsAndSelectedMeshes, aNormalizeBoneWeights), avk::content_description::bone_weight, aUsageFlags, std::move(aSyncHandler));
	}

	/** Get a buffer containing bone weights from the given input data.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@param	aSyncHandler				A synchronization handler.
	 *	@param	aNormalizeBoneWeights		Set to true to apply normalization to the bone weights, s.t. their sum equals 1.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A buffer in device memory which contains the given input data.
	 */
	template <typename... Metas>
	avk::buffer create_bone_weights_buffer_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, bool aNormalizeBoneWeights = false, vk::BufferUsageFlags aUsageFlags = {}, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		std::vector<glm::vec4> boneWeightsData;
		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			boneWeightsData = get_bone_weights(aModelsAndSelectedMeshes, aNormalizeBoneWeights);
		}
		return create_buffer_cached<std::vector<glm::vec4>, Metas...>(aSerializer, boneWeightsData, avk::content_description::bone_weight, aUsageFlags, std::move(aSyncHandler));
	}

	/**	Get bone indices from the given selection of models and associated mesh indices.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aBoneIndexOffset			Offset to be added to the bone indices.
	 *	@return	Combined bone indices data of all specified model + mesh-indices.
	 */
	extern std::vector<glm::uvec4> get_bone_indices(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aBoneIndexOffset = 0u);
	
	/**	Get bone indices from the given selection of models and associated mesh indices.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aBoneIndexOffset			Offset to be added to the bone indices.
	 *	@return	Combined bone indices data of all specified model + mesh-indices.
	 */
	extern std::vector<glm::uvec4> get_bone_indices_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aBoneIndexOffset = 0u);

	/**	Get bone indices from the given selection of models and associated mesh indices for an animation which writes bones in the "single target buffer" mode, which
	 *	means that all given meshes (as specified by their mesh indices in aModelsAndSelectedMeshes) will get their bone matrices stored in a single target buffer.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aInitialBoneIndexOffset		Offset to be added to the bone indices.
	 *	@return	Combined bone indices data of all specified model + mesh-indices.
	 */
	extern std::vector<glm::uvec4> get_bone_indices_for_single_target_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aInitialBoneIndexOffset = 0u);

	/**	Get bone indices from the given selection of models and associated mesh indices for an animation which writes bones in the "single target buffer" mode, which
	 *	means that all given meshes (as specified by their mesh indices in aModelsAndSelectedMeshes) will get their bone matrices stored in a single target buffer.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aInitialBoneIndexOffset		Offset to be added to the bone indices.
	 *	@return	Combined bone indices data of all specified model + mesh-indices.
	 */
	extern std::vector<glm::uvec4> get_bone_indices_for_single_target_buffer_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aInitialBoneIndexOffset = 0u);

	/**	Get bone indices from the given selection of models and associated mesh indices for an animation which writes bones in the "single target buffer" mode, which
	 *	means that all given meshes (as specified by their mesh indices in aModelsAndSelectedMeshes) will get their bone matrices stored in a single target buffer.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aReferenceMeshIndices		The correct offset for the given mesh index is determined based on this set. I.e. the offset will be the accumulated value
	 *										of all previous #bone-matrices in the set before the mesh with the given index.
	 *	@return	Combined bone indices data of all specified model + mesh-indices.
	 */
	extern std::vector<glm::uvec4> get_bone_indices_for_single_target_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, const std::vector<mesh_index_t>& aReferenceMeshIndices);

	/**	Get bone indices from the given selection of models and associated mesh indices for an animation which writes bones in the "single target buffer" mode, which
	 *	means that all given meshes (as specified by their mesh indices in aModelsAndSelectedMeshes) will get their bone matrices stored in a single target buffer.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aReferenceMeshIndices		The correct offset for the given mesh index is determined based on this set. I.e. the offset will be the accumulated value
	 *										of all previous #bone-matrices in the set before the mesh with the given index.
	 *	@return	Combined bone indices data of all specified model + mesh-indices.
	 */
	extern std::vector<glm::uvec4> get_bone_indices_for_single_target_buffer_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, const std::vector<mesh_index_t>& aReferenceMeshIndices);
	// TODO ^ function definition not found
	
	/**	Get a buffer containing bone indices from the given input data.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffer is created with.
	 *	@param	aSyncHandler				A synchronization handler.
	 *	@param	aBoneIndexOffset			Offset to be added to the bone indices.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A buffer in device memory which contains the given input data.
	 */
	template <typename... Metas>
	avk::buffer create_bone_indices_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aBoneIndexOffset = 0u, vk::BufferUsageFlags aUsageFlags = {}, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		return create_buffer<std::vector<glm::uvec4>, Metas...>(get_bone_indices(aModelsAndSelectedMeshes, aBoneIndexOffset), avk::content_description::bone_index, aUsageFlags, std::move(aSyncHandler));
	}

	/** Get a buffer containing bone indices from the given input data.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@param	aSyncHandler				A synchronization handler.
	 *	@param	aBoneIndexOffset			Offset to be added to the bone indices.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A buffer in device memory which contains the given input data.
	 */
	template <typename... Metas>
	avk::buffer create_bone_indices_buffer_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aBoneIndexOffset = 0u, vk::BufferUsageFlags aUsageFlags = {}, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		std::vector<glm::uvec4> boneIndicesData;
		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			boneIndicesData = get_bone_indices(aModelsAndSelectedMeshes, aBoneIndexOffset);
		}
		return create_buffer_cached<std::vector<glm::uvec4>, Metas...>(aSerializer, boneIndicesData, avk::content_description::bone_index, aUsageFlags, std::move(aSyncHandler));
	}

	/**	Get a buffer containing bone indices from the given input data.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffer is created with.
	 *	@param	aSyncHandler				A synchronization handler.
	 *	@param	aInitialBoneIndexOffset		Offset to be added to the bone indices.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A buffer in device memory which contains the given input data.
	 */
	template <typename... Metas>
	avk::buffer create_bone_indices_for_single_target_buffer_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aInitialBoneIndexOffset = 0u, vk::BufferUsageFlags aUsageFlags = {}, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		return create_buffer<std::vector<glm::uvec4>, Metas...>(get_bone_indices_for_single_target_buffer(aModelsAndSelectedMeshes, aInitialBoneIndexOffset), avk::content_description::bone_index, aUsageFlags, std::move(aSyncHandler));
	}

	/** Get a buffer containing bone indices from the given input data.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@param	aSyncHandler				A synchronization handler.
	 *	@param	aInitialBoneIndexOffset		Offset to be added to the bone indices.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A buffer in device memory which contains the given input data.
	 */
	template <typename... Metas>
	avk::buffer create_bone_indices_for_single_target_buffer_buffer_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aInitialBoneIndexOffset = 0u, vk::BufferUsageFlags aUsageFlags = {}, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		std::vector<glm::uvec4> boneIndicesData;
		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			boneIndicesData = get_bone_indices_for_single_target_buffer(aModelsAndSelectedMeshes, aInitialBoneIndexOffset);
		}
		return create_buffer_cached<std::vector<glm::uvec4>, Metas...>(aSerializer, boneIndicesData, avk::content_description::bone_index, aUsageFlags, std::move(aSyncHandler));
	}

	/**	Get a buffer containing bone indices from the given input data.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffer is created with.
	 *	@param	aSyncHandler				A synchronization handler.
	 *	@param	aReferenceMeshIndices		The correct offset for the given mesh index is determined based on this set. I.e. the offset will be the accumulated value
	 *										of all previous #bone-matrices in the set before the mesh with the given index.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A buffer in device memory which contains the given input data.
	 */
	template <typename... Metas>
	avk::buffer create_bone_indices_for_single_target_buffer_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, const std::vector<mesh_index_t>& aReferenceMeshIndices, vk::BufferUsageFlags aUsageFlags = {}, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		return create_buffer<std::vector<glm::uvec4>, Metas...>(get_bone_indices_for_single_target_buffer(aModelsAndSelectedMeshes, aReferenceMeshIndices), avk::content_description::bone_index, aUsageFlags, std::move(aSyncHandler));
	}

	/** Get a buffer containing bone indices from the given input data.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@param	aSyncHandler				A synchronization handler.
	 *	@param	aReferenceMeshIndices		The correct offset for the given mesh index is determined based on this set. I.e. the offset will be the accumulated value
	 *										of all previous #bone-matrices in the set before the mesh with the given index.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A buffer in device memory which contains the given input data.
	 */
	template <typename... Metas>
	avk::buffer create_bone_indices_for_single_target_buffer_buffer_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, const std::vector<mesh_index_t>& aReferenceMeshIndices, vk::BufferUsageFlags aUsageFlags = {}, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		std::vector<glm::uvec4> boneIndicesData;
		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			boneIndicesData = get_bone_indices_for_single_target_buffer(aModelsAndSelectedMeshes, aReferenceMeshIndices);
		}
		return create_buffer_cached<std::vector<glm::uvec4>, Metas...>(aSerializer, boneIndicesData, avk::content_description::bone_index, aUsageFlags, std::move(aSyncHandler));
	}

	/**	Get 2D texture coordinates from the given selection of models and associated mesh indices.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aTexCoordSet				The zero-based set of texture coordinates to load.
	 *	@return	Combined 2D texture coordinates data of all specified model + mesh-indices.
	 */
	extern std::vector<glm::vec2> get_2d_texture_coordinates(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet = 0);

	/**	Get 2D texture coordinates from the given selection of models and associated mesh indices.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. The order is maintained.
	 *	@param	aTexCoordSet				The zero-based set of texture coordinates to load.
	 *	@return	Combined 2D texture coordinates data of all specified model + mesh-indices tuples.
	 */
	extern std::vector<glm::vec2> get_2d_texture_coordinates_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet = 0);
	
	/**	Get 2D texture coordinates from the given selection of models and associated mesh indices, with their y-coordinates flipped.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aTexCoordSet				The zero-based set of texture coordinates to load.
	 *	@return	Combined 2D texture coordinates data of all specified model + mesh-indices.
	 */
	extern std::vector<glm::vec2> get_2d_texture_coordinates_flipped(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet = 0);
	
	/**	Get 2D texture coordinates from the given selection of models and associated mesh indices, with their y-coordinates flipped.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. The order is maintained.
	 *	@param	aTexCoordSet				The zero-based set of texture coordinates to load.
	 *	@return	Combined 2D texture coordinates data of all specified model + mesh-indices tuples.
	 */
	extern std::vector<glm::vec2> get_2d_texture_coordinates_flipped_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet = 0);
	
	/**	Get 3D texture coordinates from the given selection of models and associated mesh indices.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aTexCoordSet				The zero-based set of texture coordinates to load.
	 *	@return	Combined 3D texture coordinates data of all specified model + mesh-indices.
	 */
	extern std::vector<glm::vec3> get_3d_texture_coordinates(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet = 0);
	
	/**	Get 3D texture coordinates from the given selection of models and associated mesh indices.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. The order is maintained.
	 *	@param	aTexCoordSet				The zero-based set of texture coordinates to load.
	 *	@return	Combined 3D texture coordinates data of all specified model + mesh-indices tuples.
	 */
	extern std::vector<glm::vec3> get_3d_texture_coordinates_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet = 0);

	/**	Get a buffer containing 2D texture coordinates from the given input data.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aTexCoordSet				The zero-based set of texture coordinates to load.
	 *	@param	aUsageFlags					Additional usage flags that the buffer is created with.
	 *	@param	aSyncHandler				A synchronization handler.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A buffer in device memory which contains the given input data.
	 */
	template <typename... Metas>
	avk::buffer create_2d_texture_coordinates_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet = 0, vk::BufferUsageFlags aUsageFlags = {}, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		return create_buffer<std::vector<glm::vec2>, Metas...>(get_2d_texture_coordinates(aModelsAndSelectedMeshes, aTexCoordSet), avk::content_description::texture_coordinate, aUsageFlags, std::move(aSyncHandler));
	}

	/** Get a buffer containing 2D texture coordinates from the given input data.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aTexCoordSet				The zero-based set of texture coordinates to load.
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@param	aSyncHandler				A synchronization handler.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A buffer in device memory which contains the given input data.
	 */
	template <typename... Metas>
	avk::buffer create_2d_texture_coordinates_buffer_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet = 0, vk::BufferUsageFlags aUsageFlags = {}, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		std::vector<glm::vec2> textureCoordinatesData;
		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			textureCoordinatesData = get_2d_texture_coordinates(aModelsAndSelectedMeshes, aTexCoordSet);
		}
		return create_buffer_cached<std::vector<glm::vec2>, Metas...>(aSerializer, textureCoordinatesData, avk::content_description::texture_coordinate, aUsageFlags, std::move(aSyncHandler));
	}

	/**	Get a buffer containing 2D texture coordinates from the given input data, with their y-coordinates flipped.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aTexCoordSet				The zero-based set of texture coordinates to load.
	 *	@param	aUsageFlags					Additional usage flags that the buffer is created with.
	 *	@param	aSyncHandler				A synchronization handler.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A buffer in device memory which contains the given input data.
	 */
	template <typename... Metas>
	avk::buffer create_2d_texture_coordinates_flipped_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet = 0, vk::BufferUsageFlags aUsageFlags = {}, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		return create_buffer<std::vector<glm::vec2>, Metas...>(get_2d_texture_coordinates_flipped(aModelsAndSelectedMeshes, aTexCoordSet), avk::content_description::texture_coordinate, aUsageFlags, std::move(aSyncHandler));
	}

	/** Get a buffer containing 2D texture coordinates from the given input data, with their y-coordinates flipped.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aTexCoordSet				The zero-based set of texture coordinates to load.
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@param	aSyncHandler				A synchronization handler.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A buffer in device memory which contains the given input data.
	 */
	template <typename... Metas>
	avk::buffer create_2d_texture_coordinates_flipped_buffer_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet = 0, vk::BufferUsageFlags aUsageFlags = {}, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		std::vector<glm::vec2> textureCoordinatesData;
		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			textureCoordinatesData = get_2d_texture_coordinates_flipped(aModelsAndSelectedMeshes, aTexCoordSet);
		}
		return create_buffer_cached<std::vector<glm::vec2>, Metas...>(aSerializer, textureCoordinatesData, avk::content_description::texture_coordinate, aUsageFlags, std::move(aSyncHandler));
	}

	/**	Get a buffer containing 3D texture coordinates from the given input data.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aTexCoordSet				The zero-based set of texture coordinates to load.
	 *	@param	aUsageFlags					Additional usage flags that the buffer is created with.
	 *	@param	aSyncHandler				A synchronization handler.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A buffer in device memory which contains the given input data.
	 */
	template <typename... Metas>
	avk::buffer create_3d_texture_coordinates_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet = 0, vk::BufferUsageFlags aUsageFlags = {}, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		return create_buffer<std::vector<glm::vec3>, Metas...>(get_3d_texture_coordinates(aModelsAndSelectedMeshes, aTexCoordSet), avk::content_description::texture_coordinate, aUsageFlags, std::move(aSyncHandler));
	}

	/** Get a buffer containing 3D texture coordinates from the given input data.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aTexCoordSet				The zero-based set of texture coordinates to load.
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@param	aSyncHandler				A synchronization handler.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A buffer in device memory which contains the given input data.
	 */
	template <typename... Metas>
	avk::buffer create_3d_texture_coordinates_buffer_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet = 0, vk::BufferUsageFlags aUsageFlags = {}, avk::sync aSyncHandler = avk::sync::wait_idle())
	{
		std::vector<glm::vec3> textureCoordinatesData;
		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			textureCoordinatesData = get_3d_texture_coordinates(aModelsAndSelectedMeshes, aTexCoordSet);
		}
		return create_buffer_cached<std::vector<glm::vec3>, Metas...>(aSerializer, textureCoordinatesData, avk::content_description::texture_coordinate, aUsageFlags, std::move(aSyncHandler));
	}

	/**	Convert the given material config into a format that is usable with a GPU buffer for the materials (i.e. properly vec4-aligned),
	 *	and a set of images and samplers, which are already created on and uploaded to the GPU.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aMaterialConfigs			The material config in CPU format
	 *	@param	aLoadTexturesInSrgb			Set to true to load the images in sRGB format if applicable
	 *	@param	aFlipTextures				Set to true to y-flip images			
	 *	@param	aImageUsage					How this image is going to be used. Can be a combination of different avk::image_usage values
	 *	@param	aTextureFilterMode			Texture filtering mode for all the textures. Trilinear or anisotropic filtering modes will trigger MIP-maps to be generated.
	 *	@param	aBorderHandlingMode			Border handling mode for all the textures			
	 *	@param	aSyncHandler				A synchronization handler.
	 *	@reutrn	A tuple with two elements:
	 *			<0>: A collection of structs that contains material data converted to a GPU-suitable format. Image indices refer to the indices of the second tuple element:
	 *			<1>: A list of image samplers that were loaded from the referenced images in aMaterialConfigs, i.e. these are already actual GPU resources.
	 */
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
