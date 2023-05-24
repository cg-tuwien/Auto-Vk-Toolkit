#pragma once

#include "image_data.hpp"
#include "material_gpu_data_ext.hpp"
#include "model.hpp"
#include "serializer.hpp"
#include "context_vulkan.hpp"

namespace avk
{
	static std::tuple<avk::image, avk::command::action_type_command> create_1px_texture_cached(std::array<uint8_t, 4> aColor, avk::layout::image_layout aImageLayout, vk::Format aFormat = vk::Format::eR8G8B8A8Unorm, avk::memory_usage aMemoryUsage = avk::memory_usage::device, avk::image_usage aImageUsage = avk::image_usage::general_texture, std::optional<std::reference_wrapper<avk::serializer>> aSerializer = {})
	{
		auto stagingBuffer = context().create_buffer(
			AVK_STAGING_BUFFER_MEMORY_USAGE,
			vk::BufferUsageFlagBits::eTransferSrc,
			avk::generic_buffer_meta::create_from_size(sizeof(aColor))
		);

		if (!aSerializer) {
			auto nop = stagingBuffer->fill(aColor.data(), 0);
			assert(!nop.mBeginFun);
			assert(!nop.mEndFun);
		}
		else if (aSerializer && aSerializer->get().mode() == avk::serializer::mode::serialize) {
			auto nop = stagingBuffer->fill(aColor.data(), 0);
			assert(!nop.mBeginFun);
			assert(!nop.mEndFun);
			aSerializer->get().archive_memory(aColor.data(), sizeof(aColor));
		}
		else if (aSerializer && aSerializer->get().mode() == avk::serializer::mode::deserialize) {
			aSerializer->get().archive_buffer(*stagingBuffer);
		}

		auto img = context().create_image(1u, 1u, aFormat, 1, aMemoryUsage, aImageUsage);

		using namespace avk;
		auto actionTypeCommand = avk::command::action_type_command{
			{},
			{ // Resource-specific sync-hints (will be accumulated later into mSyncHint):
				// No need for a sync hint for the staging buffer; it is in host-visible memory. Nothing must be waited on before we can start to fill it.
				std::make_tuple(img->handle(), avk::sync::sync_hint{ // But create a sync hint for the image:
					avk::stage::none     + avk::access::none,          // No need for any dependencies to previous commands. The host-visible data is made available on the queue submit.
					avk::stage::transfer + avk::access::transfer_write // Dependency chain with the last image_memory_barrier
				})
			},
			{}, // No mBeginFun, only nested commands:
			{
				avk::sync::image_memory_barrier(*img, // No need to wait on the staging buffer since it is in host-visible memory
					avk::stage::none  >> avk::stage::copy,
					avk::access::none >> avk::access::transfer_read | avk::access::transfer_write // Just wait on the image layout transition
				).with_layout_transition(avk::layout::undefined >> avk::layout::transfer_dst),

				copy_buffer_to_image(stagingBuffer, img.get(), avk::layout::transfer_dst),

				avk::sync::image_memory_barrier(*img,
					avk::stage::copy            >> avk::stage::transfer,
					avk::access::transfer_write >> avk::access::none
				).with_layout_transition(avk::layout::transfer_dst >> aImageLayout)
			}
		};

		actionTypeCommand.infer_sync_hint_from_resource_sync_hints();

		return std::make_tuple(std::move(img), std::move(actionTypeCommand));
	}

	static std::tuple<avk::image, avk::command::action_type_command> create_1px_texture(std::array<uint8_t, 4> aColor, avk::layout::image_layout aImageLayout, vk::Format aFormat = vk::Format::eR8G8B8A8Unorm, avk::memory_usage aMemoryUsage = avk::memory_usage::device, avk::image_usage aImageUsage = avk::image_usage::general_texture)
	{
		return create_1px_texture_cached(aColor, aImageLayout, aFormat, aMemoryUsage, aImageUsage);
	}

	static std::tuple<avk::image, avk::command::action_type_command> create_1px_texture_cached(avk::serializer& aSerializer, std::array<uint8_t, 4> aColor, avk::layout::image_layout aImageLayout, vk::Format aFormat = vk::Format::eR8G8B8A8Unorm, avk::memory_usage aMemoryUsage = avk::memory_usage::device, avk::image_usage aImageUsage = avk::image_usage::general_texture)
	{
		return create_1px_texture_cached(aColor, aImageLayout, aFormat, aMemoryUsage, aImageUsage, aSerializer);
	}

	/** Create image_data from texture file
	* @param aPath					file name of a texture file to load the image data from
	* @param aLoadHdrIfPossible		load the texture as HDR (high dynamic range) data, if supported by the image loading library. If set to true, the image data may be returned in a HDR format even if the texture file does not contain HDR data. If set to false, the image data may be returned in an LDR format even if the texture contains HDR data. It is therefore advised to set this parameter according to the data format of the texture file.
	* @param aLoadSrgbIfApplicable	load the texture as sRGB color-corrected data, if supported by the image loading library. If set to true, the image data may be returned in an sRGB format even if the texture file does not contain sRGB data. If set to false, the image data may be returned in a plain RGB format even if the texture contains sRGB data. It is therefore advised to set this parameter according to the color space of the texture file.
	* @param aFlip					flip the image vertically (upside-down) if set to true. This may be needed if the layout of the image data in the texture file does not match the texture coordinates with which it is used. This parameter may not be supported for all image loaders and texture formats, in particular for some compressed textures.
	* @param aPreferredNumberOfTextureComponents	defines the number of color channels in the returned image_data. The default of 4 corresponds to RGBA texture components. A value of 1 and 2 denote grey value and grey value with alpha, respectively. If the texture file does not contain an alpha channel, the result will be fully opaque. Note that many Vulkan implementations only support textures with RGBA components. This parameter may be ignored by the image loader.
	* @return an object referencing the possibly converted and transformed image data
	*/
	static image_data get_image_data(const std::string& aPath, bool aLoadHdrIfPossible = true, bool aLoadSrgbIfApplicable = true, bool aFlip = true,
		int aPreferredNumberOfTextureComponents = 4)
	{
		image_data result(aPath, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents);

		return result;
	}

	/** Create cube map image_data from individual texture files for each face
	* @param  aPaths	a vector of file names of texture files to load the image data from. The vector must contain six file names, each specifying one side of a cube map texture, in the order +X, -X, +Y, -Y, +Z, -Z. The image data from all files must have the same dimensions and texture formats, after possible HDR and sRGB conversions.
	* @param aLoadHdrIfPossible		load the texture as HDR (high dynamic range) data, if supported by the image loading library. If set to true, the image data may be returned in a HDR format even if the texture file does not contain HDR data. If set to false, the image data may be returned in an LDR format even if the texture contains HDR data. It is therefore advised to set this parameter according to the data format of the texture file.
	* @param aLoadSrgbIfApplicable	load the texture as sRGB color-corrected data, if supported by the image loading library. If set to true, the image data may be returned in an sRGB format even if the texture file does not contain sRGB data. If set to false, the image data may be returned in a plain RGB format even if the texture contains sRGB data. It is therefore advised to set this parameter according to the color space of the texture file.
	* @param aFlip					flip the image vertically (upside-down) if set to true. This may be needed if the layout of the image data in the texture file does not match the texture coordinates with which it is used. This parameter may not be supported for all image loaders and texture formats, in particular for some compressed textures.
	* @param aPreferredNumberOfTextureComponents	defines the number of color channels in the returned image_data. The default of 4 corresponds to RGBA texture components. A value of 1 and 2 denote grey value and grey value with alpha, respectively. If the texture file does not contain an alpha channel, the result will be fully opaque. Note that many Vulkan implementations only support textures with RGBA components. This parameter may be ignored by the image loader.
	* @return an object referencing the possibly converted and transformed image data
	*/
	static image_data get_image_data(const std::vector<std::string>& aPaths, bool aLoadHdrIfPossible = true, bool aLoadSrgbIfApplicable = true, bool aFlip = true,
		int aPreferredNumberOfTextureComponents = 4)
	{
		image_data result(aPaths, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents);

		return result;
	}

	/** Create cube map from image_data, with optional caching
	* Loads image data from an image_data object or the serializer cache.
	* @param aImageData		a valid instance of image_data containing cube map image data.
	* @param aImageLayout	the image layout that the image shall be transformed into
	* @param aMemoryUsage	the intended memory usage of the returned image resource.
	* @param aImageUsage	the intended image usage of the returned image resource.
	* @param aSerializer	a serializer to use for caching data loaded from disk. 
	*/
	extern std::tuple<avk::image, avk::command::action_type_command> create_cubemap_from_image_data_cached(image_data& aImageData, avk::layout::image_layout aImageLayout = avk::layout::general, avk::memory_usage aMemoryUsage = avk::memory_usage::device,
		avk::image_usage aImageUsage = avk::image_usage::general_cube_map_texture, std::optional<std::reference_wrapper<avk::serializer>> aSerializer = {});

	/** Create cube map from image_data, with caching
	* Loads image data from an image_data object or the serializer cache.
	* @param aSerializer	a serializer to use for caching data loaded from disk. 
	* @param aImageData		a valid instance of image_data containing cube map image data.
	* @param aImageLayout	the image layout that the image shall be transformed into
	* @param aMemoryUsage	the intended memory usage of the returned image resource.
	* @param aImageUsage	the intended image usage of the returned image resource.
	*/
	static std::tuple<avk::image, avk::command::action_type_command> create_cubemap_from_image_data_cached(avk::serializer& aSerializer, image_data& aImageData, avk::layout::image_layout aImageLayout = avk::layout::general, avk::memory_usage aMemoryUsage = avk::memory_usage::device,
		avk::image_usage aImageUsage = avk::image_usage::general_cube_map_texture)
	{
		return create_cubemap_from_image_data_cached(aImageData, aImageLayout, aMemoryUsage, aImageUsage, aSerializer);
	}

	/** Create cube map from image_data
	* Loads image data from an image_data object.
	* @param aImageData		a valid instance of image_data containing cube map image data.
	* @param aImageLayout	the image layout that the image shall be transformed into
	* @param aMemoryUsage	the intended memory usage of the returned image resource.
	* @param aImageUsage	the intended image usage of the returned image resource.
	*/
	static std::tuple<avk::image, avk::command::action_type_command> create_cubemap_from_image_data(image_data& aImageData, avk::layout::image_layout aImageLayout = avk::layout::general, avk::memory_usage aMemoryUsage = avk::memory_usage::device,
		avk::image_usage aImageUsage = avk::image_usage::general_cube_map_texture)
	{
		return create_cubemap_from_image_data_cached(aImageData, aImageLayout, aMemoryUsage, aImageUsage);
	}

	/** Create cube map from a single file, with optional caching
	* Loads image data from a file or the serializer cache.
	* @param aPath			file name of a texture file to load the image data from.
	* @param aLoadHdrIfPossible		load the texture as HDR (high dynamic range) data, if supported by the image loading library. If set to true, the image data may be returned in a HDR format even if the texture file does not contain HDR data. If set to false, the image data may be returned in an LDR format even if the texture contains HDR data. It is therefore advised to set this parameter according to the data format of the texture file.
	* @param aLoadSrgbIfApplicable	load the texture as sRGB color-corrected data, if supported by the image loading library. If set to true, the image data may be returned in an sRGB format even if the texture file does not contain sRGB data. If set to false, the image data may be returned in a plain RGB format even if the texture contains sRGB data. It is therefore advised to set this parameter according to the color space of the texture file.
	* @param aFlip					flip the image vertically (upside-down) if set to true. This may be needed if the layout of the image data in the texture file does not match the texture coordinates with which it is used. This parameter may not be supported for all image loaders and texture formats, in particular for some compressed textures.
	* @param aPreferredNumberOfTextureComponents	defines the number of color channels in the returned image_data. The default of 4 corresponds to RGBA texture components. A value of 1 and 2 denote grey value and grey value with alpha, respectively. If the texture file does not contain an alpha channel, the result will be fully opaque. Note that many Vulkan implementations only support textures with RGBA components. This parameter may be ignored by the image loader.
	* @param aImageLayout	the image layout that the image shall be transformed into
	* @param aMemoryUsage	the intended memory usage of the returned image resource.
	* @param aImageUsage	the intended image usage of the returned image resource.
	* @param aSerializer	a serializer to use for caching data loaded from disk.
	*/
	extern std::tuple<avk::image, avk::command::action_type_command> create_cubemap_from_file_cached(const std::string& aPath, bool aLoadHdrIfPossible = true, bool aLoadSrgbIfApplicable = true, bool aFlip = true,
		int aPreferredNumberOfTextureComponents = 4, avk::layout::image_layout aImageLayout = avk::layout::general, avk::memory_usage aMemoryUsage = avk::memory_usage::device,
		avk::image_usage aImageUsage = avk::image_usage::general_cube_map_texture, std::optional<std::reference_wrapper<avk::serializer>> aSerializer = {});

	/** Create cube map from a single file, with caching
	* Loads image data from a file or the serializer cache.
	* @param aSerializer	a serializer to use for caching data loaded from disk.
	* @param aPath			file name of a texture file to load the image data from.
	* @param aLoadHdrIfPossible		load the texture as HDR (high dynamic range) data, if supported by the image loading library. If set to true, the image data may be returned in a HDR format even if the texture file does not contain HDR data. If set to false, the image data may be returned in an LDR format even if the texture contains HDR data. It is therefore advised to set this parameter according to the data format of the texture file.
	* @param aLoadSrgbIfApplicable	load the texture as sRGB color-corrected data, if supported by the image loading library. If set to true, the image data may be returned in an sRGB format even if the texture file does not contain sRGB data. If set to false, the image data may be returned in a plain RGB format even if the texture contains sRGB data. It is therefore advised to set this parameter according to the color space of the texture file.
	* @param aFlip					flip the image vertically (upside-down) if set to true. This may be needed if the layout of the image data in the texture file does not match the texture coordinates with which it is used. This parameter may not be supported for all image loaders and texture formats, in particular for some compressed textures.
	* @param aPreferredNumberOfTextureComponents	defines the number of color channels in the returned image_data. The default of 4 corresponds to RGBA texture components. A value of 1 and 2 denote grey value and grey value with alpha, respectively. If the texture file does not contain an alpha channel, the result will be fully opaque. Note that many Vulkan implementations only support textures with RGBA components. This parameter may be ignored by the image loader.
	* @param aImageLayout	the image layout that the image shall be transformed into
	* @param aMemoryUsage	the intended memory usage of the returned image resource.
	* @param aImageUsage	the intended image usage of the returned image resource.
	*/
	static std::tuple<avk::image, avk::command::action_type_command> create_cubemap_from_file_cached(avk::serializer& aSerializer, const std::string& aPath, bool aLoadHdrIfPossible = true, bool aLoadSrgbIfApplicable = true, bool aFlip = true,
		int aPreferredNumberOfTextureComponents = 4, avk::layout::image_layout aImageLayout = avk::layout::general, avk::memory_usage aMemoryUsage = avk::memory_usage::device,
		avk::image_usage aImageUsage = avk::image_usage::general_cube_map_texture)
	{
		return create_cubemap_from_file_cached(aPath, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents, aImageLayout, aMemoryUsage, aImageUsage, aSerializer);
	}

	/** Create cube map from a single file
	* Loads image data from a file.
	* @param aPath			file name of a texture file to load the image data from.
	* @param aLoadHdrIfPossible		load the texture as HDR (high dynamic range) data, if supported by the image loading library. If set to true, the image data may be returned in a HDR format even if the texture file does not contain HDR data. If set to false, the image data may be returned in an LDR format even if the texture contains HDR data. It is therefore advised to set this parameter according to the data format of the texture file.
	* @param aLoadSrgbIfApplicable	load the texture as sRGB color-corrected data, if supported by the image loading library. If set to true, the image data may be returned in an sRGB format even if the texture file does not contain sRGB data. If set to false, the image data may be returned in a plain RGB format even if the texture contains sRGB data. It is therefore advised to set this parameter according to the color space of the texture file.
	* @param aFlip					flip the image vertically (upside-down) if set to true. This may be needed if the layout of the image data in the texture file does not match the texture coordinates with which it is used. This parameter may not be supported for all image loaders and texture formats, in particular for some compressed textures.
	* @param aPreferredNumberOfTextureComponents	defines the number of color channels in the returned image_data. The default of 4 corresponds to RGBA texture components. A value of 1 and 2 denote grey value and grey value with alpha, respectively. If the texture file does not contain an alpha channel, the result will be fully opaque. Note that many Vulkan implementations only support textures with RGBA components. This parameter may be ignored by the image loader.
	* @param aImageLayout	the image layout that the image shall be transformed into
	* @param aMemoryUsage	the intended memory usage of the returned image resource.
	* @param aImageUsage	the intended image usage of the returned image resource.
	*/
	static std::tuple<avk::image, avk::command::action_type_command> create_cubemap_from_file(const std::string& aPath, bool aLoadHdrIfPossible = true, bool aLoadSrgbIfApplicable = true, bool aFlip = true,
		int aPreferredNumberOfTextureComponents = 4, avk::layout::image_layout aImageLayout = avk::layout::general, avk::memory_usage aMemoryUsage = avk::memory_usage::device,
		avk::image_usage aImageUsage = avk::image_usage::general_cube_map_texture)
	{
		return create_cubemap_from_file_cached(aPath, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents, aImageLayout, aMemoryUsage, aImageUsage);
	}

	/** Create cube map from six individual files, with optional caching
	* Loads image data from files or the serializer cache.
	* @param aPaths			a vector of file names of texture files to load the image data from. The vector must contain six file names, each specifying one side of a cube map texture, in the order +X, -X, +Y, -Y, +Z, -Z. The image data from all
	* @param aLoadHdrIfPossible		load the texture as HDR (high dynamic range) data, if supported by the image loading library. If set to true, the image data may be returned in a HDR format even if the texture file does not contain HDR data. If set to false, the image data may be returned in an LDR format even if the texture contains HDR data. It is therefore advised to set this parameter according to the data format of the texture file.
	* @param aLoadSrgbIfApplicable	load the texture as sRGB color-corrected data, if supported by the image loading library. If set to true, the image data may be returned in an sRGB format even if the texture file does not contain sRGB data. If set to false, the image data may be returned in a plain RGB format even if the texture contains sRGB data. It is therefore advised to set this parameter according to the color space of the texture file.
	* @param aFlip					flip the image vertically (upside-down) if set to true. This may be needed if the layout of the image data in the texture file does not match the texture coordinates with which it is used. This parameter may not be supported for all image loaders and texture formats, in particular for some compressed textures.
	* @param aPreferredNumberOfTextureComponents	defines the number of color channels in the returned image_data. The default of 4 corresponds to RGBA texture components. A value of 1 and 2 denote grey value and grey value with alpha, respectively. If the texture file does not contain an alpha channel, the result will be fully opaque. Note that many Vulkan implementations only support textures with RGBA components. This parameter may be ignored by the image loader.
	* @param aImageLayout	the image layout that the image shall be transformed into
	* @param aMemoryUsage	the intended memory usage of the returned image resource.
	* @param aImageUsage	the intended image usage of the returned image resource.
	* @param aSerializer	a serializer to use for caching data loaded from disk. 
	*/
	extern std::tuple<avk::image, avk::command::action_type_command> create_cubemap_from_file_cached(const std::vector<std::string>& aPaths, bool aLoadHdrIfPossible = true, bool aLoadSrgbIfApplicable = true, bool aFlip = true,
		int aPreferredNumberOfTextureComponents = 4, avk::layout::image_layout aImageLayout = avk::layout::general, avk::memory_usage aMemoryUsage = avk::memory_usage::device,
		avk::image_usage aImageUsage = avk::image_usage::general_cube_map_texture, std::optional<std::reference_wrapper<avk::serializer>> aSerializer = {});

	/** Create cube map from six individual files, with caching
	* Loads image data from files or the serializer cache.
	* @param aSerializer	a serializer to use for caching data loaded from disk. 
	* @param aPaths			a vector of file names of texture files to load the image data from. The vector must contain six file names, each specifying one side of a cube map texture, in the order +X, -X, +Y, -Y, +Z, -Z. The image data from all files must have the same dimensions and texture formats, after possible HDR and sRGB conversions.
	* @param aLoadHdrIfPossible		load the texture as HDR (high dynamic range) data, if supported by the image loading library. If set to true, the image data may be returned in a HDR format even if the texture file does not contain HDR data. If set to false, the image data may be returned in an LDR format even if the texture contains HDR data. It is therefore advised to set this parameter according to the data format of the texture file.
	* @param aLoadSrgbIfApplicable	load the texture as sRGB color-corrected data, if supported by the image loading library. If set to true, the image data may be returned in an sRGB format even if the texture file does not contain sRGB data. If set to false, the image data may be returned in a plain RGB format even if the texture contains sRGB data. It is therefore advised to set this parameter according to the color space of the texture file.
	* @param aFlip					flip the image vertically (upside-down) if set to true. This may be needed if the layout of the image data in the texture file does not match the texture coordinates with which it is used. This parameter may not be supported for all image loaders and texture formats, in particular for some compressed textures.
	* @param aPreferredNumberOfTextureComponents	defines the number of color channels in the returned image_data. The default of 4 corresponds to RGBA texture components. A value of 1 and 2 denote grey value and grey value with alpha, respectively. If the texture file does not contain an alpha channel, the result will be fully opaque. Note that many Vulkan implementations only support textures with RGBA components. This parameter may be ignored by the image loader.
	* @param aImageLayout	the image layout that the image shall be transformed into
	* @param aMemoryUsage	the intended memory usage of the returned image resource.
	* @param aImageUsage	the intended image usage of the returned image resource.
	*/
	static std::tuple<avk::image, avk::command::action_type_command> create_cubemap_from_file_cached(avk::serializer& aSerializer, const std::vector<std::string>& aPaths, bool aLoadHdrIfPossible = true, bool aLoadSrgbIfApplicable = true, bool aFlip = true, int aPreferredNumberOfTextureComponents = 4, avk::layout::image_layout aImageLayout = avk::layout::general, avk::memory_usage aMemoryUsage = avk::memory_usage::device, avk::image_usage aImageUsage = avk::image_usage::general_cube_map_texture)
	{
		return create_cubemap_from_file_cached(aPaths, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents, aImageLayout, aMemoryUsage, aImageUsage, aSerializer);
	}

	/** Create cube map from six individual files
	* Loads image data from files or the serializer cache.
	* @param aPaths			a vector of file names of texture files to load the image data from. The vector must contain six file names, each specifying one side of a cube map texture, in the order +X, -X, +Y, -Y, +Z, -Z. The image data from all files must have the same dimensions and texture formats, after possible HDR and sRGB conversions.
	* @param aLoadHdrIfPossible		load the texture as HDR (high dynamic range) data, if supported by the image loading library. If set to true, the image data may be returned in a HDR format even if the texture file does not contain HDR data. If set to false, the image data may be returned in an LDR format even if the texture contains HDR data. It is therefore advised to set this parameter according to the data format of the texture file.
	* @param aLoadSrgbIfApplicable	load the texture as sRGB color-corrected data, if supported by the image loading library. If set to true, the image data may be returned in an sRGB format even if the texture file does not contain sRGB data. If set to false, the image data may be returned in a plain RGB format even if the texture contains sRGB data. It is therefore advised to set this parameter according to the color space of the texture file.
	* @param aFlip					flip the image vertically (upside-down) if set to true. This may be needed if the layout of the image data in the texture file does not match the texture coordinates with which it is used. This parameter may not be supported for all image loaders and texture formats, in particular for some compressed textures.
	* @param aPreferredNumberOfTextureComponents	defines the number of color channels in the returned image_data. The default of 4 corresponds to RGBA texture components. A value of 1 and 2 denote grey value and grey value with alpha, respectively. If the texture file does not contain an alpha channel, the result will be fully opaque. Note that many Vulkan implementations only support textures with RGBA components. This parameter may be ignored by the image loader.
	* @param aImageLayout	the image layout that the image shall be transformed into
	* @param aMemoryUsage	the intended memory usage of the returned image resource.
	* @param aImageUsage	the intended image usage of the returned image resource.
	*/
	static std::tuple<avk::image, avk::command::action_type_command> create_cubemap_from_file(const std::vector<std::string>& aPaths, bool aLoadHdrIfPossible = true, bool aLoadSrgbIfApplicable = true, bool aFlip = true, int aPreferredNumberOfTextureComponents = 4, avk::layout::image_layout aImageLayout = avk::layout::general, avk::memory_usage aMemoryUsage = avk::memory_usage::device, avk::image_usage aImageUsage = avk::image_usage::general_cube_map_texture)
	{
		return create_cubemap_from_file_cached(aPaths, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents, aImageLayout, aMemoryUsage, aImageUsage);
	}

	/** Create image from image_data, with optional caching
	* Loads image data from an image_data object or the serializer cache.
	* @param aImageData		the image data to create the image from.
	* @param aImageLayout	the image layout which the resulting image shall be tranformed into
	* @param aMemoryUsage	the intended memory usage of the returned image resource.
	* @param aImageUsage	the intended image usage of the returned image resource.
	* @param aSerializer	a serializer to use for caching data loaded from disk.
	*/
	extern std::tuple<avk::image, avk::command::action_type_command> create_image_from_image_data_cached(image_data& aImageData, avk::layout::image_layout aImageLayout, avk::memory_usage aMemoryUsage = avk::memory_usage::device,
		avk::image_usage aImageUsage = avk::image_usage::general_texture, std::optional<std::reference_wrapper<avk::serializer>> aSerializer = {});

	/** Create image from image_data, with caching
	* Loads image data from an image_data object or the serializer cache.
	* @param aSerializer	a serializer to use for caching data loaded from disk.
	* @param aImageData		the image data to create the image from.
	* @param aImageLayout	the image layout which the resulting image shall be tranformed into
	* @param aMemoryUsage	the intended memory usage of the returned image resource.
	* @param aImageUsage	the intended image usage of the returned image resource.
	*/
	static std::tuple<avk::image, avk::command::action_type_command> create_image_from_image_data_cached(avk::serializer& aSerializer, image_data& aImageData, avk::layout::image_layout aImageLayout, avk::memory_usage aMemoryUsage = avk::memory_usage::device,
		avk::image_usage aImageUsage = avk::image_usage::general_texture)
	{
		return create_image_from_image_data_cached(aImageData, aImageLayout, aMemoryUsage, aImageUsage, aSerializer);
	}

	/** Create image from image_data
	* Loads image data from an image_data object.
	* @param aImageData		the image data to create the image from.
	* @param aImageLayout	the image layout that the image shall be transformed into
	* @param aMemoryUsage	the intended memory usage of the returned image resource.
	* @param aImageUsage	the intended image usage of the returned image resource.
	*/
	static std::tuple<avk::image, avk::command::action_type_command> create_image_from_image_data(image_data& aImageData, avk::layout::image_layout aImageLayout = avk::layout::general, avk::memory_usage aMemoryUsage = avk::memory_usage::device,
		avk::image_usage aImageUsage = avk::image_usage::general_texture)
	{
		return create_image_from_image_data_cached(aImageData, aImageLayout, aMemoryUsage, aImageUsage);
	}

	/** Create image from a single file, with optional caching
	* Loads image data from a file or the serializer cache.
	* @param aPath			file name of a texture file to load the image data from.
	* @param aLoadHdrIfPossible		load the texture as HDR (high dynamic range) data, if supported by the image loading library. If set to true, the image data may be returned in a HDR format even if the texture file does not contain HDR data. If set to false, the image data may be returned in an LDR format even if the texture contains HDR data. It is therefore advised to set this parameter according to the data format of the texture file.
	* @param aLoadSrgbIfApplicable	load the texture as sRGB color-corrected data, if supported by the image loading library. If set to true, the image data may be returned in an sRGB format even if the texture file does not contain sRGB data. If set to false, the image data may be returned in a plain RGB format even if the texture contains sRGB data. It is therefore advised to set this parameter according to the color space of the texture file.
	* @param aFlip					flip the image vertically (upside-down) if set to true. This may be needed if the layout of the image data in the texture file does not match the texture coordinates with which it is used. This parameter may not be supported for all image loaders and texture formats, in particular for some compressed textures.
	* @param aPreferredNumberOfTextureComponents	defines the number of color channels in the returned image_data. The default of 4 corresponds to RGBA texture components. A value of 1 and 2 denote grey value and grey value with alpha, respectively. If the texture file does not contain an alpha channel, the result will be fully opaque. Note that many Vulkan implementations only support textures with RGBA components. This parameter may be ignored by the image loader.
	* @param aImageLayout	the image layout that the image shall be transformed into
	* @param aMemoryUsage	the intended memory usage of the returned image resource.
	* @param aImageUsage	the intended image usage of the returned image resource.
	* @param aSerializer	a serializer to use for caching data loaded from disk.
	*/
	extern std::tuple<avk::image, avk::command::action_type_command> create_image_from_file_cached(const std::string& aPath, bool aLoadHdrIfPossible = true, bool aLoadSrgbIfApplicable = true, bool aFlip = true, int aPreferredNumberOfTextureComponents = 4, avk::layout::image_layout aImageLayout = avk::layout::general, avk::memory_usage aMemoryUsage = avk::memory_usage::device, avk::image_usage aImageUsage = avk::image_usage::general_texture, std::optional<std::reference_wrapper<avk::serializer>> aSerializer = {});

	/** Create image from a single file, with caching
	* Loads image data from a file or the serializer cache.
	* @param aSerializer	a serializer to use for caching data loaded from disk.
	* @param aPath			file name of a texture file to load the image data from.
	* @param aLoadHdrIfPossible		load the texture as HDR (high dynamic range) data, if supported by the image loading library. If set to true, the image data may be returned in a HDR format even if the texture file does not contain HDR data. If set to false, the image data may be returned in an LDR format even if the texture contains HDR data. It is therefore advised to set this parameter according to the data format of the texture file.
	* @param aLoadSrgbIfApplicable	load the texture as sRGB color-corrected data, if supported by the image loading library. If set to true, the image data may be returned in an sRGB format even if the texture file does not contain sRGB data. If set to false, the image data may be returned in a plain RGB format even if the texture contains sRGB data. It is therefore advised to set this parameter according to the color space of the texture file.
	* @param aFlip					flip the image vertically (upside-down) if set to true. This may be needed if the layout of the image data in the texture file does not match the texture coordinates with which it is used. This parameter may not be supported for all image loaders and texture formats, in particular for some compressed textures.
	* @param aPreferredNumberOfTextureComponents	defines the number of color channels in the returned image_data. The default of 4 corresponds to RGBA texture components. A value of 1 and 2 denote grey value and grey value with alpha, respectively. If the texture file does not contain an alpha channel, the result will be fully opaque. Note that many Vulkan implementations only support textures with RGBA components. This parameter may be ignored by the image loader.
	* @param aImageLayout	the image layout that the image shall be transformed into
	* @param aMemoryUsage	the intended memory usage of the returned image resource.
	* @param aImageUsage	the intended image usage of the returned image resource.
	*/
	static std::tuple<avk::image, avk::command::action_type_command> create_image_from_file_cached(avk::serializer& aSerializer, const std::string& aPath, bool aLoadHdrIfPossible = true, bool aLoadSrgbIfApplicable = true, bool aFlip = true, int aPreferredNumberOfTextureComponents = 4, avk::layout::image_layout aImageLayout = avk::layout::general, avk::memory_usage aMemoryUsage = avk::memory_usage::device, avk::image_usage aImageUsage = avk::image_usage::general_texture)
	{
		return create_image_from_file_cached(aPath, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents, aImageLayout, aMemoryUsage, aImageUsage, aSerializer);
	}

	/** Create image from a single file
	* Loads image data from a file.
	* @param aPath			file name of a texture file to load the image data from.
	* @param aLoadHdrIfPossible		load the texture as HDR (high dynamic range) data, if supported by the image loading library. If set to true, the image data may be returned in a HDR format even if the texture file does not contain HDR data. If set to false, the image data may be returned in an LDR format even if the texture contains HDR data. It is therefore advised to set this parameter according to the data format of the texture file.
	* @param aLoadSrgbIfApplicable	load the texture as sRGB color-corrected data, if supported by the image loading library. If set to true, the image data may be returned in an sRGB format even if the texture file does not contain sRGB data. If set to false, the image data may be returned in a plain RGB format even if the texture contains sRGB data. It is therefore advised to set this parameter according to the color space of the texture file.
	* @param aFlip					flip the image vertically (upside-down) if set to true. This may be needed if the layout of the image data in the texture file does not match the texture coordinates with which it is used. This parameter may not be supported for all image loaders and texture formats, in particular for some compressed textures.
	* @param aPreferredNumberOfTextureComponents	defines the number of color channels in the returned image_data. The default of 4 corresponds to RGBA texture components. A value of 1 and 2 denote grey value and grey value with alpha, respectively. If the texture file does not contain an alpha channel, the result will be fully opaque. Note that many Vulkan implementations only support textures with RGBA components. This parameter may be ignored by the image loader.
	* @param aImageLayout	the image layout that the image shall be transformed into
	* @param aMemoryUsage	the intended memory usage of the returned image resource.
	* @param aImageUsage	the intended image usage of the returned image resource.
	*/
	static std::tuple<avk::image, avk::command::action_type_command> create_image_from_file(const std::string& aPath, bool aLoadHdrIfPossible = true, bool aLoadSrgbIfApplicable = true, bool aFlip = true, int aPreferredNumberOfTextureComponents = 4, avk::layout::image_layout aImageLayout = avk::layout::general, avk::memory_usage aMemoryUsage = avk::memory_usage::device, avk::image_usage aImageUsage = avk::image_usage::general_texture)
	{
		return create_image_from_file_cached(aPath, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents, aImageLayout, aMemoryUsage, aImageUsage);
	}

	template <typename... Rest>
	void add_tuple_or_indices(std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aResult)
	{ }

	// Forward-declare some overloads:
	template <typename... Rest>
	void add_tuple_or_indices(std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aResult, const model& aModel, const Rest&... rest);
	template <typename... Rest>
	void add_tuple_or_indices(std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aResult, const avk::mesh_index_t& aMeshIndex, const Rest&... rest);
	template <typename... Rest>
	void add_tuple_or_indices(std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aResult, const std::vector<avk::mesh_index_t>& aMeshIndices, const Rest&... rest);

	template <typename... Rest>
	void add_tuple_or_indices(std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aResult, const model_t& aModel, const Rest&... rest)
	{
		aResult.push_back(std::forward_as_tuple(aModel, std::vector<avk::mesh_index_t>{}));
		add_tuple_or_indices(aResult, rest...);
	}

	template <typename... Rest>
	void add_tuple_or_indices(std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aResult, const model& aModel, const Rest&... rest)
	{
		// Convenience overload to also support owning_resource references:
		add_tuple_or_indices(aResult, aModel.get(), rest...);
	}

	template <typename... Rest>
	void add_tuple_or_indices(std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aResult, const avk::mesh_index_t& aMeshIndex, const Rest&... rest)
	{
		std::get<std::vector<avk::mesh_index_t>>(aResult.back()).emplace_back(aMeshIndex);
		add_tuple_or_indices(aResult, rest...);
	}

	template <typename... Rest>
	void add_tuple_or_indices(std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aResult, const std::vector<avk::mesh_index_t>& aMeshIndices, const Rest&... rest)
	{
		auto& idxes = std::get<std::vector<avk::mesh_index_t>>(aResult.back());
		idxes.insert(std::end(idxes), std::begin(aMeshIndices), std::end(aMeshIndices));
		add_tuple_or_indices(aResult, rest...);
	}
	
	/**	This is a convenience method that allows to compile a selection of models and mesh indices.
	 *	Valid usage means passing a model as parameter, and following it up with one or multiple mesh index parameters.
	 *
	 *	Model parameters must be bindable by const model&.
	 *	Mesh index parameters are supported in the forms:
	 *	 - avk::mesh_index_t
	 *	 - std::vector<avk::mesh_index_t>
	 *
	 *	Examples of valid parameters:
	 *	 - make_models_and_meshes_selection(myModel, 1, 2, 3); // => a model and 3x avk::mesh_index_t
	 *	 - make_models_and_meshes_selection(myModel, 1, std::vector<avk::mesh_index_t>{ 2, 3 }); // => a model and then 1x avk::mesh_index_t, and 1x vector of avk::mesh_index_t (containing two indices)
	 *	 - make_models_and_meshes_selection(myModel, 1, myOtherModel, std::vector<avk::mesh_index_t>{ 0, 1 }); // => a model and then 1x avk::mesh_index_t; another model and 1x vector of avk::mesh_index_t (containing two indices)
	 */
	template <typename... Args>
	std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>> make_model_references_and_mesh_indices_selection(const Args&... args)
	{
		std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>> result;
		add_tuple_or_indices(result, args...);
		return result;
	}

	template <typename... Rest>
	void add_tuple_or_indices_shared(std::vector<std::tuple<avk::model, std::vector<mesh_index_t>>>& aResult)
	{ }

	// Forward-declare some of the overloads:
	template <typename... Rest>
	void add_tuple_or_indices_shared(std::vector<std::tuple<avk::model, std::vector<mesh_index_t>>>& aResult, avk::mesh_index_t& aMeshIndex, Rest&... rest);
	template <typename... Rest>
	void add_tuple_or_indices_shared(std::vector<std::tuple<avk::model, std::vector<mesh_index_t>>>& aResult, std::vector<avk::mesh_index_t>& aMeshIndices, Rest&... rest);

	template <typename... Rest>
	void add_tuple_or_indices_shared(std::vector<std::tuple<avk::model, std::vector<mesh_index_t>>>& aResult, model& aModel, Rest&... rest)
	{
		aResult.push_back(std::forward_as_tuple(aModel, std::vector<avk::mesh_index_t>{}));
		add_tuple_or_indices_shared(aResult, rest...);
	}

	template <typename... Rest>
	void add_tuple_or_indices_shared(std::vector<std::tuple<avk::model, std::vector<mesh_index_t>>>& aResult, avk::mesh_index_t& aMeshIndex, Rest&... rest)
	{
		std::get<std::vector<avk::mesh_index_t>>(aResult.back()).emplace_back(aMeshIndex);
		add_tuple_or_indices_shared(aResult, rest...);
	}

	template <typename... Rest>
	void add_tuple_or_indices_shared(std::vector<std::tuple<avk::model, std::vector<mesh_index_t>>>& aResult, std::vector<avk::mesh_index_t>& aMeshIndices, Rest&... rest)
	{
		auto& idxes = std::get<std::vector<avk::mesh_index_t>>(aResult.back());
		idxes.insert(std::end(idxes), std::begin(aMeshIndices), std::end(aMeshIndices));
		add_tuple_or_indices_shared(aResult, rest...);
	}

	/** This is a convenience function that allows to compile a selection of models and associated mesh indices.
	 *	Valid usage means passing a model as parameter, and following it up with one or multiple mesh index parameters.
	 *
	 *	Shared ownership will be enabled on the models.
	 *	Mesh index parameters are supported in the forms:
	 *	 - avk::mesh_index_t
	 *	 - std::vector<avk::mesh_index_t>
	 *
	 *	Examples of valid parameters:
	 *	 - make_selection_of_shared_models_and_mesh_indices(myModel, 1, 2, 3); // => a model and 3x avk::mesh_index_t
	 *	 - make_selection_of_shared_models_and_mesh_indices(myModel, 1, std::vector<avk::mesh_index_t>{ 2, 3 }); // => a model and then 1x avk::mesh_index_t, and 1x vector of avk::mesh_index_t (containing two indices)
	 *	 - make_selection_of_shared_models_and_mesh_indices(myModel, 1, myOtherModel, std::vector<avk::mesh_index_t>{ 0, 1 }); // => a model and then 1x avk::mesh_index_t; another model and 1x vector of avk::mesh_index_t (containing two indices)
	 */
	template <typename... Args>
	std::vector<std::tuple<avk::model, std::vector<mesh_index_t>>> make_models_and_mesh_indices_selection(Args&... args)
	{
		std::vector<std::tuple<avk::model, std::vector<mesh_index_t>>> result;
		add_tuple_or_indices_shared(result, args...);
		return result;
	}

	/** Helper function to fill a given device buffer with a staging buffer whose contents are loaded from a cache file.
	 *	@param	aSerializer		The serializer in avk::serializer::mode::deserialize
	 *	@param  aQueue			The queue to submit the commands to
	 *	@param	aDeviceBuffer	The target buffer
	 *	@param	aTotalSize		Size of the staging buffer
	 */
	static inline avk::command::action_type_command fill_device_buffer_from_cache(avk::serializer& aSerializer, avk::resource_argument<avk::buffer_t> aDeviceBuffer, size_t aTotalSize)
	{
		assert(aSerializer.mode() == avk::serializer::mode::deserialize);
		
		// Create host visible staging buffer for filling on host side from file
		auto sb = context().create_buffer(
			AVK_STAGING_BUFFER_MEMORY_USAGE,
			vk::BufferUsageFlagBits::eTransferSrc,
			avk::generic_buffer_meta::create_from_size(aTotalSize)
		);

		// Let the serializer map and fill the buffer
		aSerializer.archive_buffer(*sb);
		
		auto actionTypeCommand = avk::copy_buffer_to_another(std::move(sb), std::move(aDeviceBuffer), 0, 0, aTotalSize);
		return actionTypeCommand;
	}

	/**	Get a tuple of <0>:vertices and <1>:indices from the given selection of models and associated mesh indices.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@return	Combined position and index data of all specified model + mesh-indices tuples, where the returned tuple's elements refer to:
	 *			<0>: vertex positions
	 *			<1>: indices
	 */
	extern std::tuple<std::vector<glm::vec3>, std::vector<uint32_t>> get_vertices_and_indices(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes);

	/**	Get a tuple of <0>:vertices and <1>:indices from the given selection of models and associated mesh indices.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. The order is maintained.
	 *	@return	Combined position and index data of all specified model + mesh-indices tuples, where the returned tuple's elements refer to:
	 *			<0>: vertex positions
	 *			<1>: indices
	 */
	extern std::tuple<std::vector<glm::vec3>, std::vector<uint32_t>> get_vertices_and_indices_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes);

	// A concept which requires a type to have a .describe_member(size_t aOffset, content_description aContent)
	template <typename T>
	concept has_describe_member = requires (T x)
	{
		x.describe_member(size_t{ 0 }, vk::Format::eUndefined, avk::content_description::position);
	};

	// A concept which requires a type to have a .set_format<glm::uvec3>(avk::content_description)
	template <typename T>
	concept has_set_format_for_index_buffer = requires (T x)
	{
		x.template set_format<glm::uvec3>(avk::content_description::index);
	};

	// Helper which creates meta data for uniform/storage_texel_buffer_view metas:
	template <typename Meta>
	auto set_up_meta_from_data_for_index_buffer(const std::vector<uint32_t>& aIndicesData) requires has_set_format_for_index_buffer<Meta>
	{
		return Meta::create_from_data(aIndicesData).template set_format<glm::uvec3>(avk::content_description::index); // Combine 3 consecutive elements to one unit
	}

	// ...and another helper which creates metas for other Meta types:
	template <typename Meta>
	auto set_up_meta_from_data_for_index_buffer(const std::vector<uint32_t>& aIndicesData) requires (!has_set_format_for_index_buffer<Meta>)
	{
		return Meta::create_from_data(aIndicesData);
	}

	// Helper which creates meta data for uniform/storage_texel_buffer_view metas:
	template <typename Meta>
	auto set_up_meta_from_data_for_vertex_buffer(const std::vector<glm::vec3>& aVerticesData) requires has_describe_member<Meta>
	{
		return Meta::create_from_data(aVerticesData).describe_member(0, avk::format_for<std::remove_reference_t<decltype(aVerticesData)>::value_type>(), avk::content_description::position);
	}

	// ...and another helper which creates metas for other Meta types:
	template <typename Meta>
	auto set_up_meta_from_data_for_vertex_buffer(const std::vector<glm::vec3>& aVerticesData) requires (!has_describe_member<Meta>)
	{
		return Meta::create_from_data(aVerticesData);
	}

	// Helper which creates meta data for uniform/storage_texel_buffer_view from size specification
	template <typename Meta>
	auto set_up_meta_from_total_size_for_index_buffer(size_t aTotalSize, size_t aNumElements) requires has_set_format_for_index_buffer<Meta>
	{
		return Meta::create_from_total_size(aTotalSize, aNumElements).template set_format<glm::uvec3>(avk::content_description::index); // Combine 3 consecutive elements to one unit
	}

	// ...and another helper which creates metas for other Meta types:
	template <typename Meta>
	auto set_up_meta_from_total_size_for_index_buffer(size_t aTotalSize, size_t aNumElements) requires (!has_set_format_for_index_buffer<Meta>)
	{
		return Meta::create_from_total_size(aTotalSize, aNumElements);
	}

	// Helper which creates meta data for uniform/storage_texel_buffer_view from size specification
	template <typename Meta, typename FMT>
	auto set_up_meta_from_total_size_for_vertex_buffer(size_t aTotalSize, size_t aNumElements) requires has_describe_member<Meta>
	{
		return Meta::create_from_total_size(aTotalSize, aNumElements).describe_member(0, avk::format_for<FMT>(), avk::content_description::position);
	}

	// ...and another helper which creates metas for other Meta types:
	template <typename Meta, typename FMT>
	auto set_up_meta_from_total_size_for_vertex_buffer(size_t aTotalSize, size_t aNumElements) requires (!has_describe_member<Meta>)
	{
		return Meta::create_from_total_size(aTotalSize, aNumElements);
	}
	
	// Helper which creates meta data for a given data + content description, but only if the Meta type in question has a describe_member member:
	template <typename T, typename Meta>
	auto set_up_meta_for_data_with_or_without_describe_member(const T& aData, avk::content_description aContentDescription) requires has_describe_member<Meta>
	{
		return Meta::create_from_data(aData).describe_member(0, avk::format_for<typename std::remove_reference_t<decltype(aData)>::value_type>(), aContentDescription);
	}

	// Helper which creates meta data for a given data + content description, but only if the Meta type in question has a describe_member member:
	template <typename T, typename Meta>
	auto set_up_meta_for_data_with_or_without_describe_member(const T& aData, avk::content_description aContentDescription) requires (!has_describe_member<Meta>)
	{
		return Meta::create_from_data(aData);
	}

	// Helper which creates meta data for a given data + size specifications, but only if the Meta type in question has a describe_member member:
	template <typename T, typename Meta>
	auto set_up_meta_from_total_size_with_or_without_describe_member(const T& aData, size_t bufferTotalSize, size_t numBufferEntries, avk::content_description aContentDescription) requires has_describe_member<Meta>
	{
		return Meta::create_from_total_size(bufferTotalSize, numBufferEntries).describe_member(0, avk::format_for<typename std::remove_reference_t<decltype(aData)>::value_type>(), aContentDescription);
	}

	// Helper which creates meta data for a given data + size specifications, but only if the Meta type in question has a describe_member member:
	template <typename T, typename Meta>
	auto set_up_meta_from_total_size_with_or_without_describe_member(const T& aData, size_t bufferTotalSize, size_t numBufferEntries, avk::content_description aContentDescription) requires (!has_describe_member<Meta>)
	{
		return Meta::create_from_total_size(bufferTotalSize, numBufferEntries);
	}

	/**	Get a tuple of two buffers, containing vertex positions and index positions, respectively, from the given input data.
	 *	@param	aVerticesAndIndices			A tuple containing vertex positions data in the first element, and index data in the second element.
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of each buffer besides their obligatory
	 *										avk::vertex_buffer_meta, and avk::index_buffer_meta, as appropriate for the two buffers.
	 *										The additional meta data declarations will always refer to the whole data in the buffers; specifying subranges is not supported.
	 *	@return	A tuple of two buffers in device memory which contain the given input data, where the returned tuple's elements refer to:
	 *			<0>: Buffer containing vertex positions. Attention: The user of this function must ensure that it is not destroyed before the returned commands (at tuple index <2>) have completed execution.
	 *			<1>: Buffer containing indices.          Attention: The user of this function must ensure that it is not destroyed before the returned commands (at tuple index <2>) have completed execution.
	 *			<2>: Commands that need to be executed on a queue to complete the operation.
	 */
	template <typename... Metas>
	std::tuple<avk::buffer, avk::buffer, avk::command::action_type_command> create_vertex_and_index_buffers(const std::tuple<std::vector<glm::vec3>, std::vector<uint32_t>>& aVerticesAndIndices, vk::BufferUsageFlags aUsageFlags = {})
	{
		const auto& [positionsData, indicesData] = aVerticesAndIndices;
		avk::command::action_type_command actionTypeCommand{};

		auto positionsBuffer = context().create_buffer(
			avk::memory_usage::device, aUsageFlags,
			avk::vertex_buffer_meta::create_from_data(positionsData).describe_member(0, avk::format_for<typename std::remove_reference_t<decltype(positionsData)>::value_type>(), avk::content_description::position),
			set_up_meta_from_data_for_vertex_buffer<Metas>(positionsData)...
		);
		
		actionTypeCommand.mNestedCommandsAndSyncInstructions.push_back(positionsBuffer->fill(positionsData.data(), 0));

		// It is fine to let positionsData go out of scope, since its data has been copied to a
		// staging buffer within fill, which is lifetime-handled by the command buffer.

		auto indexBuffer = context().create_buffer(
			avk::memory_usage::device, aUsageFlags,
			avk::index_buffer_meta::create_from_data(indicesData),
			set_up_meta_from_data_for_index_buffer<Metas>(indicesData)...
		);

		actionTypeCommand.mNestedCommandsAndSyncInstructions.push_back(indexBuffer->fill(indicesData.data(), 0));
		actionTypeCommand.infer_sync_hint_from_nested_commands();
		
		return std::make_tuple(std::move(positionsBuffer), std::move(indexBuffer), std::move(actionTypeCommand));
	}
	
	/**	Get a tuple of two buffers, containing vertex positions and index positions, respectively, from the given input data.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of each buffer besides their obligatory
	 *										avk::vertex_buffer_meta, and avk::index_buffer_meta, as appropriate for the two buffers.
	 *										The additional meta data declarations will always refer to the whole data in the buffers; specifying subranges is not supported.
	 *	@return	A tuple of two buffers in device memory which contain the given input data, where the returned tuple's elements refer to:
	 *			<0>: buffer containing vertex positions
	 *			<1>: buffer containing indices
	 *			<2>: commands that need to be executed on a queue to complete the operation
	 */
	template <typename... Metas>
	std::tuple<avk::buffer, avk::buffer, avk::command::action_type_command> create_vertex_and_index_buffers(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, vk::BufferUsageFlags aUsageFlags = {})
	{
		return create_vertex_and_index_buffers<Metas...>(get_vertices_and_indices(aModelsAndSelectedMeshes), aUsageFlags);
	}
	
	/**	Get a tuple of two buffers, containing vertex positions and index positions, respectively, from the given input data.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aVerticesAndIndices			A tuple containing vertex positions data in the first element, and index data in the second element.
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of each buffer besides their obligatory
	 *										avk::vertex_buffer_meta, and avk::index_buffer_meta, as appropriate for the two buffers.
	 *										The additional meta data declarations will always refer to the whole data in the buffers; specifying subranges is not supported.
	 *	@return	A tuple of two buffers in device memory which contain the given input data, where the tuple elements refer to:
	 *			<0>: Buffer containing vertex positions. Attention: The user of this function must ensure that it is not destroyed before the returned commands (at tuple index <2>) have completed execution.
	 *			<1>: Buffer containing indices.          Attention: The user of this function must ensure that it is not destroyed before the returned commands (at tuple index <2>) have completed execution.
	 *			<2>: Commands that need to be executed on a queue to complete the operation.
	 */
	template <typename... Metas>
	std::tuple<avk::buffer, avk::buffer, avk::command::action_type_command> create_vertex_and_index_buffers_cached(avk::serializer& aSerializer, std::tuple<std::vector<glm::vec3>, std::vector<uint32_t>>& aVerticesAndIndices, vk::BufferUsageFlags aUsageFlags = {})
	{
		size_t numPositions = 0;
		size_t totalPositionsSize = 0;
		size_t numIndices = 0;
		size_t totalIndicesSize = 0;

		auto& [positionsData, indicesData] = aVerticesAndIndices;
		if (aSerializer.mode() == avk::serializer::mode::serialize) {
			numPositions = positionsData.size();
			totalPositionsSize = sizeof(std::remove_reference_t<decltype(positionsData)>::value_type) * numPositions;
			numIndices = indicesData.size();
			totalIndicesSize = sizeof(std::remove_reference_t<decltype(indicesData)>::value_type) * numIndices;

			aSerializer.archive(numPositions);
			aSerializer.archive(totalPositionsSize);
			aSerializer.archive(numIndices);
			aSerializer.archive(totalIndicesSize);

			aSerializer.archive_memory(positionsData.data(), totalPositionsSize);
			aSerializer.archive_memory(indicesData.data(), totalIndicesSize);

			return create_vertex_and_index_buffers<Metas...>(std::make_tuple(std::move(positionsData), std::move(indicesData)), aUsageFlags);
		}
		else {
			aSerializer.archive(numPositions);
			aSerializer.archive(totalPositionsSize);
			aSerializer.archive(numIndices);
			aSerializer.archive(totalIndicesSize);

			auto positionsBuffer = context().create_buffer(
				avk::memory_usage::device, aUsageFlags,
				avk::vertex_buffer_meta::create_from_total_size(totalPositionsSize, numPositions).describe_member(0, avk::format_for<typename std::remove_reference_t<decltype(positionsData)>::value_type>(), avk::content_description::position),
				set_up_meta_from_total_size_for_vertex_buffer<Metas, std::remove_reference_t<decltype(positionsData)>::value_type>(totalPositionsSize, numPositions)...
			);

			avk::command::action_type_command actionTypeCommand{};
			actionTypeCommand.mNestedCommandsAndSyncInstructions.push_back(fill_device_buffer_from_cache(aSerializer, positionsBuffer, totalPositionsSize));

			auto indexBuffer = context().create_buffer(
				avk::memory_usage::device, aUsageFlags,
				avk::index_buffer_meta::create_from_total_size(totalIndicesSize, numIndices),
				set_up_meta_from_total_size_for_index_buffer<Metas>(totalIndicesSize, numIndices)...
			);

			actionTypeCommand.mNestedCommandsAndSyncInstructions.push_back(fill_device_buffer_from_cache(aSerializer, indexBuffer, totalIndicesSize));
			actionTypeCommand.infer_sync_hint_from_nested_commands();
			
			return std::make_tuple(std::move(positionsBuffer), std::move(indexBuffer), std::move(actionTypeCommand));
		}
	}

	/**	Get a tuple of two buffers, containing vertex positions and index positions, respectively, from the given input data.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of each buffer besides their obligatory
	 *										avk::vertex_buffer_meta, and avk::index_buffer_meta, as appropriate for the two buffers.
	 *										The additional meta data declarations will always refer to the whole data in the buffers; specifying subranges is not supported.
	 *	@return	A tuple of two buffers in device memory which contain the given input data, where the tuple elements refer to:
	 *			<0>: Buffer containing vertex positions. Attention: The user of this function must ensure that it is not destroyed before the returned commands (at tuple index <2>) have completed execution.
	 *			<1>: Buffer containing indices.          Attention: The user of this function must ensure that it is not destroyed before the returned commands (at tuple index <2>) have completed execution.
	 *			<2>: Commands that need to be executed on a queue to complete the operation.
	 */
	template <typename... Metas>
	std::tuple<avk::buffer, avk::buffer, avk::command::action_type_command> create_vertex_and_index_buffers_cached(avk::serializer& aSerializer, std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, vk::BufferUsageFlags aUsageFlags = {})
	{
		std::tuple<std::vector<glm::vec3>, std::vector<uint32_t>> verticesAndIndicesData;
		if (aSerializer.mode() == avk::serializer::mode::serialize) {
			verticesAndIndicesData = get_vertices_and_indices(aModelsAndSelectedMeshes);
		}
		return create_vertex_and_index_buffers_cached<Metas...>(aSerializer, verticesAndIndicesData, aUsageFlags);
	}

	/**	Create a device buffer that contains the given input data
	 *	@param	aBufferData					Data to be stored in the buffer
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A tuple containing the following values:
	 *			<0>: A buffer in device memory which will contain the given input data. Attention: The user of this function must ensure that it is not destroyed before the returned commands (at tuple index <1>) have completed execution.
	 *			<1>: Commands that need to be executed on a queue to complete the operation
	 */
	template <typename T, typename... Metas>
	std::tuple<avk::buffer, avk::command::action_type_command> create_buffer(const T& aBufferData, vk::BufferUsageFlags aUsageFlags = {})
	{
		auto buffer = context().create_buffer(
			avk::memory_usage::device, aUsageFlags,
			avk::generic_buffer_meta::create_from_data(aBufferData),
			Metas::create_from_data(aBufferData)...
		);

		auto actionTypeCommand = buffer->fill(aBufferData.data(), 0);
		return std::make_tuple(std::move(buffer), std::move(actionTypeCommand));
	}

	/**	Create a device buffer that contains the given input data
	 *	@param	aBufferData					Data to be stored in the buffer
	 *	@param	aContentDescription			Description of the buffer's content
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A tuple containing the following values:
	 *			<0>: A buffer in device memory which will contain the given input data. Attention: The user of this function must ensure that it is not destroyed before the returned commands (at tuple index <1>) have completed execution.
	 *			<1>: Commands that need to be executed on a queue to complete the operation
	 */
	template <typename T, typename... Metas>
	std::tuple<avk::buffer, avk::command::action_type_command> create_buffer(const T& aBufferData, avk::content_description aContentDescription, vk::BufferUsageFlags aUsageFlags = {})
	{
		auto buffer = context().create_buffer(
			avk::memory_usage::device, aUsageFlags,
			avk::generic_buffer_meta::create_from_data(aBufferData),
			set_up_meta_for_data_with_or_without_describe_member<T, Metas>(aBufferData, aContentDescription)...
		);

		auto actionTypeCommand = buffer->fill(aBufferData.data(), 0);
		return std::make_tuple(std::move(buffer), std::move(actionTypeCommand));

	}

	/** Create a device buffer that contains the given input data
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aBufferData					Data to be stored in the buffer
	 *	@param	aContentDescription			Description of the buffer's content
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A tuple containing the following values:
	 *			<0>: A buffer in device memory which will contain the given input data. Attention: The user of this function must ensure that it is not destroyed before the returned commands (at tuple index <1>) have completed execution.
	 *			<1>: Commands that need to be executed on a queue to complete the operation
	 */
	template <typename T, typename... Metas>
	std::tuple<avk::buffer, avk::command::action_type_command> create_buffer_cached(avk::serializer& aSerializer, T& aBufferData, avk::content_description aContentDescription, vk::BufferUsageFlags aUsageFlags = {})
	{
		size_t numBufferEntries = 0;
		size_t bufferTotalSize = 0;

		if (aSerializer.mode() == avk::serializer::mode::serialize) {
			numBufferEntries = aBufferData.size();
			bufferTotalSize = sizeof(typename std::remove_reference_t<decltype(aBufferData)>::value_type) * numBufferEntries;

			aSerializer.archive(numBufferEntries);
			aSerializer.archive(bufferTotalSize);

			aSerializer.archive_memory(aBufferData.data(), bufferTotalSize);

			return create_buffer<T, Metas...>(aBufferData, aContentDescription, aUsageFlags);
		}
		else {
			aSerializer.archive(numBufferEntries);
			aSerializer.archive(bufferTotalSize);

			auto buffer = context().create_buffer(
				avk::memory_usage::device, aUsageFlags,
				avk::generic_buffer_meta::create_from_size(bufferTotalSize),
				set_up_meta_from_total_size_with_or_without_describe_member<T, Metas>(aBufferData, bufferTotalSize, numBufferEntries, aContentDescription)...
			);

			auto actionTypeCommand = fill_device_buffer_from_cache(aSerializer, buffer, bufferTotalSize);
			return std::make_tuple(std::move(buffer), std::move(actionTypeCommand));
		}
	}
	
	/**	Get normals from the given selection of models and associated mesh indices.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@return	Combined normals data of all specified model + mesh-indices.
	 */
	extern std::vector<glm::vec3> get_normals(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes);

	/**	Get normals from the given selection of models and associated mesh indices.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. The order is maintained.
	 *	@return	Combined normals data of all specified model + mesh-indices tuples.
	 */
	extern std::vector<glm::vec3> get_normals_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes);

	/**	Get a buffer containing normals from the given input data.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffer is created with.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A tuple containing the following values:
	 *			<0>: A buffer in device memory which will contain the normals. Attention: The user of this function must ensure that it is not destroyed before the returned commands (at tuple index <1>) have completed execution.
	 *			<1>: Commands that need to be executed on a queue to complete the operation
	 */
	template <typename... Metas>
	std::tuple<avk::buffer, avk::command::action_type_command> create_normals_buffer(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, vk::BufferUsageFlags aUsageFlags = {})
	{
		return create_buffer<std::vector<glm::vec3>, Metas...>(get_normals(aModelsAndSelectedMeshes), avk::content_description::normal, aUsageFlags);
	}

	/** Get a buffer containing normals from the given input data.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A tuple containing the following values:
	 *			<0>: A buffer in device memory which will contain the normals. Attention: The user of this function must ensure that it is not destroyed before the returned commands (at tuple index <1>) have completed execution.
	 *			<1>: Commands that need to be executed on a queue to complete the operation
	 */
	template <typename... Metas>
	std::tuple<avk::buffer, avk::command::action_type_command> create_normals_buffer_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, vk::BufferUsageFlags aUsageFlags = {})
	{
		std::vector<glm::vec3> normalsData;
		if (aSerializer.mode() == avk::serializer::mode::serialize) {
			normalsData = get_normals(aModelsAndSelectedMeshes);
		}
		return create_buffer_cached<std::vector<glm::vec3>, Metas...>(aSerializer, normalsData, avk::content_description::normal, aUsageFlags);
	}

	/**	Get tangents from the given selection of models and associated mesh indices.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@return	Combined tangents data of all specified model + mesh-indices.
	 */
	extern std::vector<glm::vec3> get_tangents(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes);

	/**	Get tangents from the given selection of models and associated mesh indices.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. The order is maintained.
	 *	@return	Combined tangents data of all specified model + mesh-indices tuples.
	 */
	extern std::vector<glm::vec3> get_tangents_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes);
	
	/**	Get a buffer containing tangents from the given input data.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffer is created with.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A tuple containing the following values:
	 *			<0>: A buffer in device memory which will contain the tangents. Attention: The user of this function must ensure that it is not destroyed before the returned commands (at tuple index <1>) have completed execution.
	 *			<1>: Commands that need to be executed on a queue to complete the operation
	 */
	template <typename... Metas>
	std::tuple<avk::buffer, avk::command::action_type_command> create_tangents_buffer(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, vk::BufferUsageFlags aUsageFlags = {})
	{
		return create_buffer<std::vector<glm::vec3>, Metas...>(get_tangents(aModelsAndSelectedMeshes), avk::content_description::tangent, aUsageFlags);
	}

	/** Get a buffer containing tangents from the given input data.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A tuple containing the following values:
	 *			<0>: A buffer in device memory which will contain the tangents. Attention: The user of this function must ensure that it is not destroyed before the returned commands (at tuple index <1>) have completed execution.
	 *			<1>: Commands that need to be executed on a queue to complete the operation
	 */
	template <typename... Metas>
	std::tuple<avk::buffer, avk::command::action_type_command> create_tangents_buffer_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, vk::BufferUsageFlags aUsageFlags = {})
	{
		std::vector<glm::vec3> tangentsData;
		if (aSerializer.mode() == avk::serializer::mode::serialize) {
			tangentsData = get_tangents(aModelsAndSelectedMeshes);
		}
		return create_buffer_cached<std::vector<glm::vec3>, Metas...>(aSerializer, tangentsData, avk::content_description::tangent, aUsageFlags);
	}
	
	/**	Get bitangents from the given selection of models and associated mesh indices.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@return	Combined bitangents data of all specified model + mesh-indices.
	 */
	extern std::vector<glm::vec3> get_bitangents(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes);

	/**	Get bitangents from the given selection of models and associated mesh indices.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. The order is maintained.
	 *	@return	Combined bitangents data of all specified model + mesh-indices tuples.
	 */
	extern std::vector<glm::vec3> get_bitangents_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes);

	/**	Get a buffer containing bitangents from the given input data.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffer is created with.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A tuple containing the following values:
	 *			<0>: A buffer in device memory which will contain the bitangents. Attention: The user of this function must ensure that it is not destroyed before the returned commands (at tuple index <1>) have completed execution.
	 *			<1>: Commands that need to be executed on a queue to complete the operation
	 */
	template <typename... Metas>
	std::tuple<avk::buffer, avk::command::action_type_command> create_bitangents_buffer(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, vk::BufferUsageFlags aUsageFlags = {})
	{
		return create_buffer<std::vector<glm::vec3>, Metas...>(get_bitangents(aModelsAndSelectedMeshes), avk::content_description::bitangent, aUsageFlags);
	}

	/** Get a buffer containing bitangents from the given input data.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A tuple containing the following values:
	 *			<0>: A buffer in device memory which will contain the given input data. Attention: The user of this function must ensure that it is not destroyed before the returned commands (at tuple index <1>) have completed execution.
	 *			<1>: Commands that need to be executed on a queue to complete the operation
	 */
	template <typename... Metas>
	std::tuple<avk::buffer, avk::command::action_type_command> create_bitangents_buffer_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, vk::BufferUsageFlags aUsageFlags = {})
	{
		std::vector<glm::vec3> bitangentsData;
		if (aSerializer.mode() == avk::serializer::mode::serialize) {
			bitangentsData = get_bitangents(aModelsAndSelectedMeshes);
		}
		return create_buffer_cached<std::vector<glm::vec3>, Metas...>(aSerializer, bitangentsData, avk::content_description::bitangent, aUsageFlags);
	}

	/**	Get colors from the given selection of models and associated mesh indices.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aColorsSet					The zero-based set of vertex colors to get
	 *	@return	Combined colors data of all specified model + mesh-indices.
	 */
	extern std::vector<glm::vec4> get_colors(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, int aColorsSet = 0);

	/**	Get colors from the given selection of models and associated mesh indices.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. The order is maintained.
	 *	@param	aColorsSet					The zero-based set of vertex colors to get
	 *	@return	Combined colors data of all specified model + mesh-indices tuples.
	 */
	extern std::vector<glm::vec4> get_colors_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, int aColorsSet = 0);
	
	/**	Get a buffer containing colors from the given input data.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffer is created with.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A tuple containing the following values:
	 *			<0>: A buffer in device memory which will contain the given input data. Attention: The user of this function must ensure that it is not destroyed before the returned commands (at tuple index <1>) have completed execution.
	 *			<1>: Commands that need to be executed on a queue to complete the operation
	 */
	template <typename... Metas>
	std::tuple<avk::buffer, avk::command::action_type_command> create_colors_buffer(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, int aColorsSet = 0, vk::BufferUsageFlags aUsageFlags = {})
	{
		return create_buffer<std::vector<glm::vec4>, Metas...>(get_colors(aModelsAndSelectedMeshes, aColorsSet), avk::content_description::color, aUsageFlags);
	}

	/** Get a buffer containing colors from the given input data.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A tuple containing the following values:
	 *			<0>: A buffer in device memory which will contain the colors. Attention: The user of this function must ensure that it is not destroyed before the returned commands (at tuple index <1>) have completed execution.
	 *			<1>: Commands that need to be executed on a queue to complete the operation
	 */
	template <typename... Metas>
	std::tuple<avk::buffer, avk::command::action_type_command> create_colors_buffer_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, int aColorsSet = 0, vk::BufferUsageFlags aUsageFlags = {})
	{
		std::vector<glm::vec4> colorsData;
		if (aSerializer.mode() == avk::serializer::mode::serialize) {
			colorsData = get_colors(aModelsAndSelectedMeshes, aColorsSet);
		}
		return create_buffer_cached<std::vector<glm::vec4>, Metas...>(aSerializer, colorsData, avk::content_description::color, aUsageFlags);
	}

	/**	Get bone weights from the given selection of models and associated mesh indices.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aNormalizeBoneWeights		Set to true to apply normalization to the bone weights, s.t. their sum equals 1.
	 *	@return	Combined normals data of all specified model + mesh-indices.
	 */
	extern std::vector<glm::vec4> get_bone_weights(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, bool aNormalizeBoneWeights = false);

	/**	Get bone weights from the given selection of models and associated mesh indices.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. The order is maintained.
	 *	@param	aNormalizeBoneWeights		Set to true to apply normalization to the bone weights, s.t. their sum equals 1.
	 *	@return	Combined normals data of all specified model + mesh-indices tuples.
	 */
	extern std::vector<glm::vec4> get_bone_weights_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, bool aNormalizeBoneWeights = false);

	/**	Get a buffer containing bone weights from the given input data.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffer is created with.
	 *	@param	aNormalizeBoneWeights		Set to true to apply normalization to the bone weights, s.t. their sum equals 1.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A tuple containing the following values:
	 *			<0>: A buffer in device memory which will contain the bone weights. Attention: The user of this function must ensure that it is not destroyed before the returned commands (at tuple index <1>) have completed execution.
	 *			<1>: Commands that need to be executed on a queue to complete the operation
	 */
	template <typename... Metas>
	std::tuple<avk::buffer, avk::command::action_type_command> create_bone_weights_buffer(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, bool aNormalizeBoneWeights = false, vk::BufferUsageFlags aUsageFlags = {})
	{
		return create_buffer<std::vector<glm::vec4>, Metas...>(get_bone_weights(aModelsAndSelectedMeshes, aNormalizeBoneWeights), avk::content_description::bone_weight, aUsageFlags);
	}

	/** Get a buffer containing bone weights from the given input data.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@param	aNormalizeBoneWeights		Set to true to apply normalization to the bone weights, s.t. their sum equals 1.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A tuple containing the following values:
	 *			<0>: A buffer in device memory which will contain the given input data. Attention: The user of this function must ensure that it is not destroyed before the returned commands (at tuple index <1>) have completed execution.
	 *			<1>: Commands that need to be executed on a queue to complete the operation
	 */
	template <typename... Metas>
	std::tuple<avk::buffer, avk::command::action_type_command> create_bone_weights_buffer_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, bool aNormalizeBoneWeights = false, vk::BufferUsageFlags aUsageFlags = {})
	{
		std::vector<glm::vec4> boneWeightsData;
		if (aSerializer.mode() == avk::serializer::mode::serialize) {
			boneWeightsData = get_bone_weights(aModelsAndSelectedMeshes, aNormalizeBoneWeights);
		}
		return create_buffer_cached<std::vector<glm::vec4>, Metas...>(aSerializer, boneWeightsData, avk::content_description::bone_weight, aUsageFlags);
	}

	/**	Get bone indices from the given selection of models and associated mesh indices.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aBoneIndexOffset			Offset to be added to the bone indices.
	 *	@return	Combined bone indices data of all specified model + mesh-indices.
	 */
	extern std::vector<glm::uvec4> get_bone_indices(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aBoneIndexOffset = 0u);
	
	/**	Get bone indices from the given selection of models and associated mesh indices.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aBoneIndexOffset			Offset to be added to the bone indices.
	 *	@return	Combined bone indices data of all specified model + mesh-indices.
	 */
	extern std::vector<glm::uvec4> get_bone_indices_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aBoneIndexOffset = 0u);

	/**	Get bone indices from the given selection of models and associated mesh indices for an animation which writes bones in the "single target buffer" mode, which
	 *	means that all given meshes (as specified by their mesh indices in aModelsAndSelectedMeshes) will get their bone matrices stored in a single target buffer.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aInitialBoneIndexOffset		Offset to be added to the bone indices.
	 *	@return	Combined bone indices data of all specified model + mesh-indices.
	 */
	extern std::vector<glm::uvec4> get_bone_indices_for_single_target_buffer(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aInitialBoneIndexOffset = 0u);

	/**	Get bone indices from the given selection of models and associated mesh indices for an animation which writes bones in the "single target buffer" mode, which
	 *	means that all given meshes (as specified by their mesh indices in aModelsAndSelectedMeshes) will get their bone matrices stored in a single target buffer.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aInitialBoneIndexOffset		Offset to be added to the bone indices.
	 *	@return	Combined bone indices data of all specified model + mesh-indices.
	 */
	extern std::vector<glm::uvec4> get_bone_indices_for_single_target_buffer_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aInitialBoneIndexOffset = 0u);

	/**	Get bone indices from the given selection of models and associated mesh indices for an animation which writes bones in the "single target buffer" mode, which
	 *	means that all given meshes (as specified by their mesh indices in aModelsAndSelectedMeshes) will get their bone matrices stored in a single target buffer.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aReferenceMeshIndices		The correct offset for the given mesh index is determined based on this set. I.e. the offset will be the accumulated value
	 *										of all previous #bone-matrices in the set before the mesh with the given index.
	 *	@return	Combined bone indices data of all specified model + mesh-indices.
	 */
	extern std::vector<glm::uvec4> get_bone_indices_for_single_target_buffer(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, const std::vector<mesh_index_t>& aReferenceMeshIndices);

	/**	Get bone indices from the given selection of models and associated mesh indices for an animation which writes bones in the "single target buffer" mode, which
	 *	means that all given meshes (as specified by their mesh indices in aModelsAndSelectedMeshes) will get their bone matrices stored in a single target buffer.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aReferenceMeshIndices		The correct offset for the given mesh index is determined based on this set. I.e. the offset will be the accumulated value
	 *										of all previous #bone-matrices in the set before the mesh with the given index.
	 *	@return	Combined bone indices data of all specified model + mesh-indices.
	 */
	extern std::vector<glm::uvec4> get_bone_indices_for_single_target_buffer_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, const std::vector<mesh_index_t>& aReferenceMeshIndices);
	// TODO ^ function definition not found
	
	/**	Get a buffer containing bone indices from the given input data.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffer is created with.
	 *	@param	aBoneIndexOffset			Offset to be added to the bone indices.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A tuple containing the following values:
	 *			<0>: A buffer in device memory which will contain the bone indices. Attention: The user of this function must ensure that it is not destroyed before the returned commands (at tuple index <1>) have completed execution.
	 *			<1>: Commands that need to be executed on a queue to complete the operation
	 */
	template <typename... Metas>
	std::tuple<avk::buffer, avk::command::action_type_command> create_bone_indices_buffer(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aBoneIndexOffset = 0u, vk::BufferUsageFlags aUsageFlags = {})
	{
		return create_buffer<std::vector<glm::uvec4>, Metas...>(get_bone_indices(aModelsAndSelectedMeshes, aBoneIndexOffset), avk::content_description::bone_index, aUsageFlags);
	}

	/** Get a buffer containing bone indices from the given input data.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@param	aBoneIndexOffset			Offset to be added to the bone indices.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A tuple containing the following values:
	 *			<0>: A buffer in device memory which will contain the given input data. Attention: The user of this function must ensure that it is not destroyed before the returned commands (at tuple index <1>) have completed execution.
	 *			<1>: Commands that need to be executed on a queue to complete the operation
	 */
	template <typename... Metas>
	std::tuple<avk::buffer, avk::command::action_type_command> create_bone_indices_buffer_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aBoneIndexOffset = 0u, vk::BufferUsageFlags aUsageFlags = {})
	{
		std::vector<glm::uvec4> boneIndicesData;
		if (aSerializer.mode() == avk::serializer::mode::serialize) {
			boneIndicesData = get_bone_indices(aModelsAndSelectedMeshes, aBoneIndexOffset);
		}
		return create_buffer_cached<std::vector<glm::uvec4>, Metas...>(aSerializer, boneIndicesData, avk::content_description::bone_index, aUsageFlags);
	}

	/**	Get a buffer containing bone indices from the given input data.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffer is created with.
	 *	@param	aInitialBoneIndexOffset		Offset to be added to the bone indices.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A buffer in device memory which contains the given input data.
	 */
	template <typename... Metas>
	avk::buffer create_bone_indices_for_single_target_buffer_buffer(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aInitialBoneIndexOffset = 0u, vk::BufferUsageFlags aUsageFlags = {})
	{
		return create_buffer<std::vector<glm::uvec4>, Metas...>(get_bone_indices_for_single_target_buffer(aModelsAndSelectedMeshes, aInitialBoneIndexOffset), avk::content_description::bone_index, aUsageFlags);
	}

	/** Get a buffer containing bone indices from the given input data.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@param	aInitialBoneIndexOffset		Offset to be added to the bone indices.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A buffer in device memory which contains the given input data.
	 */
	template <typename... Metas>
	avk::buffer create_bone_indices_for_single_target_buffer_buffer_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aInitialBoneIndexOffset = 0u, vk::BufferUsageFlags aUsageFlags = {})
	{
		std::vector<glm::uvec4> boneIndicesData;
		if (aSerializer.mode() == avk::serializer::mode::serialize) {
			boneIndicesData = get_bone_indices_for_single_target_buffer(aModelsAndSelectedMeshes, aInitialBoneIndexOffset);
		}
		return create_buffer_cached<std::vector<glm::uvec4>, Metas...>(aSerializer, boneIndicesData, avk::content_description::bone_index, aUsageFlags);
	}

	/**	Get a buffer containing bone indices from the given input data.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffer is created with.
	 *	@param	aReferenceMeshIndices		The correct offset for the given mesh index is determined based on this set. I.e. the offset will be the accumulated value
	 *										of all previous #bone-matrices in the set before the mesh with the given index.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A buffer in device memory which contains the given input data.
	 */
	template <typename... Metas>
	avk::buffer create_bone_indices_for_single_target_buffer_buffer(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, const std::vector<mesh_index_t>& aReferenceMeshIndices, vk::BufferUsageFlags aUsageFlags = {})
	{
		return create_buffer<std::vector<glm::uvec4>, Metas...>(get_bone_indices_for_single_target_buffer(aModelsAndSelectedMeshes, aReferenceMeshIndices), avk::content_description::bone_index, aUsageFlags);
	}

	/** Get a buffer containing bone indices from the given input data.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@param	aReferenceMeshIndices		The correct offset for the given mesh index is determined based on this set. I.e. the offset will be the accumulated value
	 *										of all previous #bone-matrices in the set before the mesh with the given index.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A buffer in device memory which contains the given input data.
	 */
	template <typename... Metas>
	avk::buffer create_bone_indices_for_single_target_buffer_buffer_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, const std::vector<mesh_index_t>& aReferenceMeshIndices, vk::BufferUsageFlags aUsageFlags = {})
	{
		std::vector<glm::uvec4> boneIndicesData;
		if (aSerializer.mode() == avk::serializer::mode::serialize) {
			boneIndicesData = get_bone_indices_for_single_target_buffer(aModelsAndSelectedMeshes, aReferenceMeshIndices);
		}
		return create_buffer_cached<std::vector<glm::uvec4>, Metas...>(aSerializer, boneIndicesData, avk::content_description::bone_index, aUsageFlags);
	}

	/**	Get 2D texture coordinates from the given selection of models and associated mesh indices.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aTexCoordSet				The zero-based set of texture coordinates to load.
	 *	@return	Combined 2D texture coordinates data of all specified model + mesh-indices.
	 */
	extern std::vector<glm::vec2> get_2d_texture_coordinates(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet = 0);

	/**	Get 2D texture coordinates from the given selection of models and associated mesh indices.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. The order is maintained.
	 *	@param	aTexCoordSet				The zero-based set of texture coordinates to load.
	 *	@return	Combined 2D texture coordinates data of all specified model + mesh-indices tuples.
	 */
	extern std::vector<glm::vec2> get_2d_texture_coordinates_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet = 0);
	
	/**	Get 2D texture coordinates from the given selection of models and associated mesh indices, with their y-coordinates flipped.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aTexCoordSet				The zero-based set of texture coordinates to load.
	 *	@return	Combined 2D texture coordinates data of all specified model + mesh-indices.
	 */
	extern std::vector<glm::vec2> get_2d_texture_coordinates_flipped(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet = 0);
	
	/**	Get 2D texture coordinates from the given selection of models and associated mesh indices, with their y-coordinates flipped.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. The order is maintained.
	 *	@param	aTexCoordSet				The zero-based set of texture coordinates to load.
	 *	@return	Combined 2D texture coordinates data of all specified model + mesh-indices tuples.
	 */
	extern std::vector<glm::vec2> get_2d_texture_coordinates_flipped_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet = 0);
	
	/**	Get 3D texture coordinates from the given selection of models and associated mesh indices.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aTexCoordSet				The zero-based set of texture coordinates to load.
	 *	@return	Combined 3D texture coordinates data of all specified model + mesh-indices.
	 */
	extern std::vector<glm::vec3> get_3d_texture_coordinates(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet = 0);
	
	/**	Get 3D texture coordinates from the given selection of models and associated mesh indices.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. The order is maintained.
	 *	@param	aTexCoordSet				The zero-based set of texture coordinates to load.
	 *	@return	Combined 3D texture coordinates data of all specified model + mesh-indices tuples.
	 */
	extern std::vector<glm::vec3> get_3d_texture_coordinates_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet = 0);

	/**	Get a buffer containing 2D texture coordinates from the given input data.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aTexCoordSet				The zero-based set of texture coordinates to load.
	 *	@param	aUsageFlags					Additional usage flags that the buffer is created with.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A tuple containing the following values:
	 *			<0>: A buffer in device memory which will contain the given texture coordinates. Attention: The user of this function must ensure that it is not destroyed before the returned commands (at tuple index <1>) have completed execution.
	 *			<1>: Commands that need to be executed on a queue to complete the operation
	 */
	template <typename... Metas>
	std::tuple<avk::buffer, avk::command::action_type_command> create_2d_texture_coordinates_buffer(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet = 0, vk::BufferUsageFlags aUsageFlags = {})
	{
		return create_buffer<std::vector<glm::vec2>, Metas...>(get_2d_texture_coordinates(aModelsAndSelectedMeshes, aTexCoordSet), avk::content_description::texture_coordinate, aUsageFlags);
	}

	/** Get a buffer containing 2D texture coordinates from the given input data.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aTexCoordSet				The zero-based set of texture coordinates to load.
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A tuple containing the following values:
	 *			<0>: A buffer in device memory which will contain the given texture coordinates. Attention: The user of this function must ensure that it is not destroyed before the returned commands (at tuple index <1>) have completed execution.
	 *			<1>: Commands that need to be executed on a queue to complete the operation
	 */
	template <typename... Metas>
	std::tuple<avk::buffer, avk::command::action_type_command> create_2d_texture_coordinates_buffer_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet = 0, vk::BufferUsageFlags aUsageFlags = {})
	{
		std::vector<glm::vec2> textureCoordinatesData;
		if (aSerializer.mode() == avk::serializer::mode::serialize) {
			textureCoordinatesData = get_2d_texture_coordinates(aModelsAndSelectedMeshes, aTexCoordSet);
		}
		return create_buffer_cached<std::vector<glm::vec2>, Metas...>(aSerializer, textureCoordinatesData, avk::content_description::texture_coordinate, aUsageFlags);
	}

	/**	Get a buffer containing 2D texture coordinates from the given input data, with their y-coordinates flipped.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aTexCoordSet				The zero-based set of texture coordinates to load.
	 *	@param	aUsageFlags					Additional usage flags that the buffer is created with.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A tuple containing the following values:
	 *			<0>: A buffer in device memory which will contain the given texture coordinates. Attention: The user of this function must ensure that it is not destroyed before the returned commands (at tuple index <1>) have completed execution.
	 *			<1>: Commands that need to be executed on a queue to complete the operation
	 */
	template <typename... Metas>
	std::tuple<avk::buffer, avk::command::action_type_command> create_2d_texture_coordinates_flipped_buffer(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet = 0, vk::BufferUsageFlags aUsageFlags = {})
	{
		return create_buffer<std::vector<glm::vec2>, Metas...>(get_2d_texture_coordinates_flipped(aModelsAndSelectedMeshes, aTexCoordSet), avk::content_description::texture_coordinate, aUsageFlags);
	}

	/** Get a buffer containing 2D texture coordinates from the given input data, with their y-coordinates flipped.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aTexCoordSet				The zero-based set of texture coordinates to load.
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A tuple containing the following values:
	 *			<0>: A buffer in device memory which will contain the given texture coordinates. Attention: The user of this function must ensure that it is not destroyed before the returned commands (at tuple index <1>) have completed execution.
	 *			<1>: Commands that need to be executed on a queue to complete the operation
	 */
	template <typename... Metas>
	std::tuple<avk::buffer, avk::command::action_type_command> create_2d_texture_coordinates_flipped_buffer_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet = 0, vk::BufferUsageFlags aUsageFlags = {})
	{
		std::vector<glm::vec2> textureCoordinatesData;
		if (aSerializer.mode() == avk::serializer::mode::serialize) {
			textureCoordinatesData = get_2d_texture_coordinates_flipped(aModelsAndSelectedMeshes, aTexCoordSet);
		}
		return create_buffer_cached<std::vector<glm::vec2>, Metas...>(aSerializer, textureCoordinatesData, avk::content_description::texture_coordinate, aUsageFlags);
	}

	/**	Get a buffer containing 3D texture coordinates from the given input data.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aTexCoordSet				The zero-based set of texture coordinates to load.
	 *	@param	aUsageFlags					Additional usage flags that the buffer is created with.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A tuple containing the following values:
	 *			<0>: A buffer in device memory which will contain the given texture coordinates. Attention: The user of this function must ensure that it is not destroyed before the returned commands (at tuple index <1>) have completed execution.
	 *			<1>: Commands that need to be executed on a queue to complete the operation
	 */
	template <typename... Metas>
	std::tuple<avk::buffer, avk::command::action_type_command> create_3d_texture_coordinates_buffer(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet = 0, vk::BufferUsageFlags aUsageFlags = {})
	{
		return create_buffer<std::vector<glm::vec3>, Metas...>(get_3d_texture_coordinates(aModelsAndSelectedMeshes, aTexCoordSet), avk::content_description::texture_coordinate, aUsageFlags);
	}

	/** Get a buffer containing 3D texture coordinates from the given input data.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aModelsAndSelectedMeshes	A collection where every entry consists of a model-reference + associated mesh indices.
	 *										All the data they refer to is combined into a a common result. Their order is maintained.
	 *	@param	aTexCoordSet				The zero-based set of texture coordinates to load.
	 *	@param	aUsageFlags					Additional usage flags that the buffers are created with.
	 *	@tparam	Metas						A list of buffer meta data types which shall be added to the creation of the buffer.
	 *										The additional meta data declarations will always refer to the whole data in the buffer; specifying subranges is not supported.
	 *	@return	A tuple containing the following values:
	 *			<0>: A buffer in device memory which will contain the given texture coordinates. Attention: The user of this function must ensure that it is not destroyed before the returned commands (at tuple index <1>) have completed execution.
	 *			<1>: Commands that need to be executed on a queue to complete the operation
	 */
	template <typename... Metas>
	std::tuple<avk::buffer, avk::command::action_type_command> create_3d_texture_coordinates_buffer_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet = 0, vk::BufferUsageFlags aUsageFlags = {})
	{
		std::vector<glm::vec3> textureCoordinatesData;
		if (aSerializer.mode() == avk::serializer::mode::serialize) {
			textureCoordinatesData = get_3d_texture_coordinates(aModelsAndSelectedMeshes, aTexCoordSet);
		}
		return create_buffer_cached<std::vector<glm::vec3>, Metas...>(aSerializer, textureCoordinatesData, avk::content_description::texture_coordinate, aUsageFlags);
	}
	
	/**	Create a new sampler with the given configuration parameters
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aFilterMode					Filtering strategy for the sampler to be created
	 *	@param	aBorderHandlingModes		Border handling strategy for the sampler to be created for u, v, and w coordinates (in that order)
	 *	@param	aMipMapMaxLod				Default value = house number
	 *	@param	aAlterConfigBeforeCreation	A context-specific function which allows to alter the configuration before the sampler is created.
	 */
	avk::sampler create_sampler_cached(avk::serializer& aSerializer, avk::filter_mode aFilterMode, std::array<avk::border_handling_mode, 3> aBorderHandlingModes, float aMipMapMaxLod = VK_LOD_CLAMP_NONE, std::function<void(avk::sampler_t&)> aAlterConfigBeforeCreation = {});

	/**	Create a new sampler with the given configuration parameters
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aFilterMode					Filtering strategy for the sampler to be created
	 *	@param	aBorderHandlingModes		Border handling strategy for the sampler to be created for u, and v coordinates (in that order). (The w direction will get the same strategy assigned as v)
	 *	@param	aMipMapMaxLod				Default value = house number
	 *	@param	aAlterConfigBeforeCreation	A context-specific function which allows to alter the configuration before the sampler is created.
	 */
	avk::sampler create_sampler_cached(avk::serializer& aSerializer, avk::filter_mode aFilterMode, std::array<avk::border_handling_mode, 2> aBorderHandlingModes, float aMipMapMaxLod = VK_LOD_CLAMP_NONE, std::function<void(avk::sampler_t&)> aAlterConfigBeforeCreation = {});

	/**	Create a new sampler with the given configuration parameters
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aFilterMode					Filtering strategy for the sampler to be created
	 *	@param	aBorderHandlingMode			Border handling strategy for all coordinates u, v, and w.
	 *	@param	aMipMapMaxLod				Default value = house number
	 *	@param	aAlterConfigBeforeCreation	A context-specific function which allows to alter the configuration before the sampler is created.
	 */
	avk::sampler create_sampler_cached(avk::serializer& aSerializer, avk::filter_mode aFilterMode, avk::border_handling_mode aBorderHandlingMode, float aMipMapMaxLod = VK_LOD_CLAMP_NONE, std::function<void(avk::sampler_t&)> aAlterConfigBeforeCreation = {});

	/**	Convert the given material config into a format that is usable with a GPU buffer for the materials (i.e. properly vec4-aligned),
	 *	and a set of images and samplers, which are already created on and uploaded to the GPU.
	 *	@param	aMaterialConfigs			The material config in CPU format
	 *	@param	aLoadTexturesInSrgb			Set to true to load the images in sRGB format if applicable
	 *	@param	aFlipTextures				Set to true to y-flip images
	 *	@param	aImageUsage					How this image is going to be used. Can be a combination of different avk::image_usage values
	 *	@param	aTextureFilterMode			Texture filtering mode for all the textures. Trilinear or anisotropic filtering modes will trigger MIP-maps to be generated.
	 *	@return	A tuple of three elements:
	 *			<0>: A collection of structs that contains material data converted to a GPU-suitable format. Image indices refer to the indices of the second tuple element:
	 *			<1>: A list of image samplers that were loaded from the referenced images in aMaterialConfigs, i.e. these are already actual GPU resources.
	 *			<2>: Zero, one, or multiple commands that need to be executed to complete the operation. This can include operations like uploading image data, or creating mip maps.
	 */
	template <typename T>
	std::tuple<std::vector<T>, std::vector<avk::image_sampler>, avk::command::action_type_command> convert_for_gpu_usage_cached(
		const std::vector<avk::material_config>& aMaterialConfigs,
		bool aLoadTexturesInSrgb,
		bool aFlipTextures,
		avk::image_usage aImageUsage,
		avk::filter_mode aTextureFilterMode,
		std::optional<std::reference_wrapper<avk::serializer>> aSerializer = {})
	{
		avk::command::action_type_command commandsToReturn{};

		// These are the texture names loaded from file -> mapped to vector of usage-pointers
		std::unordered_map<std::string, std::vector<std::tuple<std::array<avk::border_handling_mode, 2>, std::vector<int*>>>> texNamesToBorderHandlingToUsages;

		auto addTexUsage = [&texNamesToBorderHandlingToUsages](const std::string& bPath, const std::array<avk::border_handling_mode, 2>& bBhMode, int* bUsage) {
			auto& vct = texNamesToBorderHandlingToUsages[bPath];
			for (auto& [existingBhModes, usages] : vct) {
				if (existingBhModes[0] == bBhMode[0] && existingBhModes[1] == bBhMode[1]) {
					usages.push_back(bUsage);
					return; // Found => done
				}
			}
			// Not found => add new
			vct.emplace_back(bBhMode, std::vector<int*>{ bUsage });
		};

		// Textures contained in this array shall be loaded into an sRGB format
		std::set<std::string> srgbTextures;

		// However, if some textures are missing, provide 1x1 px textures in those spots
		std::vector<int*> whiteTexUsages;				// Provide a 1x1 px almost everywhere in those cases,
		std::vector<int*> straightUpNormalTexUsages;	// except for normal maps, provide a normal pointing straight up there.

		std::vector<T> result;
		if (!aSerializer ||
			(aSerializer && (aSerializer->get().mode() == serializer::mode::serialize))) {
			size_t materialConfigSize = aMaterialConfigs.size();
			result.reserve(materialConfigSize); // important because of the pointers

			for (auto& mc : aMaterialConfigs) {
				auto& newEntry = result.emplace_back();
				if constexpr (std::is_convertible<T&, material_gpu_data&>::value) {
					material_gpu_data& mgd = static_cast<material_gpu_data&>(newEntry);
					mgd.mDiffuseReflectivity = mc.mDiffuseReflectivity;
					mgd.mAmbientReflectivity = mc.mAmbientReflectivity;
					mgd.mSpecularReflectivity = mc.mSpecularReflectivity;
					mgd.mEmissiveColor = mc.mEmissiveColor;
					mgd.mTransparentColor = mc.mTransparentColor;
					mgd.mReflectiveColor = mc.mReflectiveColor;
					mgd.mAlbedo = mc.mAlbedo;

					mgd.mOpacity = mc.mOpacity;
					mgd.mBumpScaling = mc.mBumpScaling;
					mgd.mShininess = mc.mShininess;
					mgd.mShininessStrength = mc.mShininessStrength;

					mgd.mRefractionIndex = mc.mRefractionIndex;
					mgd.mReflectivity = mc.mReflectivity;
					mgd.mMetallic = mc.mMetallic;
					mgd.mSmoothness = mc.mSmoothness;

					mgd.mSheen = mc.mSheen;
					mgd.mThickness = mc.mThickness;
					mgd.mRoughness = mc.mRoughness;
					mgd.mAnisotropy = mc.mAnisotropy;

					mgd.mAnisotropyRotation = mc.mAnisotropyRotation;
					mgd.mCustomData = mc.mCustomData;

					mgd.mDiffuseTexIndex = -1;
					if (mc.mDiffuseTex.empty()) {
						whiteTexUsages.push_back(&mgd.mDiffuseTexIndex);
					}
					else {
						auto path = avk::clean_up_path(mc.mDiffuseTex);
						addTexUsage(path, mc.mDiffuseTexBorderHandlingMode, &mgd.mDiffuseTexIndex);
						if (aLoadTexturesInSrgb) {
							srgbTextures.insert(path);
						}
					}

					mgd.mSpecularTexIndex = -1;
					if (mc.mSpecularTex.empty()) {
						whiteTexUsages.push_back(&mgd.mSpecularTexIndex);
					}
					else {
						addTexUsage(avk::clean_up_path(mc.mSpecularTex), mc.mSpecularTexBorderHandlingMode, &mgd.mSpecularTexIndex);
					}

					mgd.mAmbientTexIndex = -1;
					if (mc.mAmbientTex.empty()) {
						whiteTexUsages.push_back(&mgd.mAmbientTexIndex);
					}
					else {
						auto path = avk::clean_up_path(mc.mAmbientTex);
						addTexUsage(path, mc.mAmbientTexBorderHandlingMode, &mgd.mAmbientTexIndex);
						if (aLoadTexturesInSrgb) {
							srgbTextures.insert(path);
						}
					}

					mgd.mEmissiveTexIndex = -1;
					if (mc.mEmissiveTex.empty()) {
						whiteTexUsages.push_back(&mgd.mEmissiveTexIndex);
					}
					else {
						addTexUsage(avk::clean_up_path(mc.mEmissiveTex), mc.mEmissiveTexBorderHandlingMode, &mgd.mEmissiveTexIndex);
					}

					mgd.mHeightTexIndex = -1;
					if (mc.mHeightTex.empty()) {
						whiteTexUsages.push_back(&mgd.mHeightTexIndex);
					}
					else {
						addTexUsage(avk::clean_up_path(mc.mHeightTex), mc.mHeightTexBorderHandlingMode, &mgd.mHeightTexIndex);
					}

					mgd.mNormalsTexIndex = -1;
					if (mc.mNormalsTex.empty()) {
						straightUpNormalTexUsages.push_back(&mgd.mNormalsTexIndex);
					}
					else {
						addTexUsage(avk::clean_up_path(mc.mNormalsTex), mc.mNormalsTexBorderHandlingMode, &mgd.mNormalsTexIndex);
					}

					mgd.mShininessTexIndex = -1;
					if (mc.mShininessTex.empty()) {
						whiteTexUsages.push_back(&mgd.mShininessTexIndex);
					}
					else {
						addTexUsage(avk::clean_up_path(mc.mShininessTex), mc.mShininessTexBorderHandlingMode, &mgd.mShininessTexIndex);
					}

					mgd.mOpacityTexIndex = -1;
					if (mc.mOpacityTex.empty()) {
						whiteTexUsages.push_back(&mgd.mOpacityTexIndex);
					}
					else {
						addTexUsage(avk::clean_up_path(mc.mOpacityTex), mc.mOpacityTexBorderHandlingMode, &mgd.mOpacityTexIndex);
					}

					mgd.mDisplacementTexIndex = -1;
					if (mc.mDisplacementTex.empty()) {
						whiteTexUsages.push_back(&mgd.mDisplacementTexIndex);
					}
					else {
						addTexUsage(avk::clean_up_path(mc.mDisplacementTex), mc.mDisplacementTexBorderHandlingMode, &mgd.mDisplacementTexIndex);
					}

					mgd.mReflectionTexIndex = -1;
					if (mc.mReflectionTex.empty()) {
						whiteTexUsages.push_back(&mgd.mReflectionTexIndex);
					}
					else {
						addTexUsage(avk::clean_up_path(mc.mReflectionTex), mc.mReflectionTexBorderHandlingMode, &mgd.mReflectionTexIndex);
					}

					mgd.mLightmapTexIndex = -1;
					if (mc.mLightmapTex.empty()) {
						whiteTexUsages.push_back(&mgd.mLightmapTexIndex);
					}
					else {
						addTexUsage(avk::clean_up_path(mc.mLightmapTex), mc.mLightmapTexBorderHandlingMode, &mgd.mLightmapTexIndex);
					}

					mgd.mExtraTexIndex = -1;
					if (mc.mExtraTex.empty()) {
						whiteTexUsages.push_back(&mgd.mExtraTexIndex);
					}
					else {
						auto path = avk::clean_up_path(mc.mExtraTex);
						addTexUsage(path, mc.mExtraTexBorderHandlingMode, &mgd.mExtraTexIndex);
						if (aLoadTexturesInSrgb) {
							srgbTextures.insert(path);
						}
					}

					mgd.mDiffuseTexOffsetTiling			= mc.mDiffuseTexOffsetTiling;
					mgd.mSpecularTexOffsetTiling		= mc.mSpecularTexOffsetTiling;
					mgd.mAmbientTexOffsetTiling			= mc.mAmbientTexOffsetTiling;
					mgd.mEmissiveTexOffsetTiling		= mc.mEmissiveTexOffsetTiling;
					mgd.mHeightTexOffsetTiling			= mc.mHeightTexOffsetTiling;
					mgd.mNormalsTexOffsetTiling			= mc.mNormalsTexOffsetTiling;
					mgd.mShininessTexOffsetTiling		= mc.mShininessTexOffsetTiling;
					mgd.mOpacityTexOffsetTiling			= mc.mOpacityTexOffsetTiling;
					mgd.mDisplacementTexOffsetTiling	= mc.mDisplacementTexOffsetTiling;
					mgd.mReflectionTexOffsetTiling		= mc.mReflectionTexOffsetTiling;
					mgd.mLightmapTexOffsetTiling		= mc.mLightmapTexOffsetTiling;
					mgd.mExtraTexOffsetTiling			= mc.mExtraTexOffsetTiling;
				}
				if constexpr (std::is_convertible<T&, material_gpu_data_ext&>::value) {
					material_gpu_data_ext& ext = static_cast<material_gpu_data_ext&>(newEntry);

					ext.mDiffuseTexUvSet				= mc.mDiffuseTexUvSet;
					ext.mSpecularTexUvSet				= mc.mSpecularTexUvSet;
					ext.mAmbientTexUvSet				= mc.mAmbientTexUvSet;
					ext.mEmissiveTexUvSet				= mc.mEmissiveTexUvSet;
					ext.mHeightTexUvSet					= mc.mHeightTexUvSet;
					ext.mNormalsTexUvSet				= mc.mNormalsTexUvSet;
					ext.mShininessTexUvSet				= mc.mShininessTexUvSet;
					ext.mOpacityTexUvSet				= mc.mOpacityTexUvSet;
					ext.mDisplacementTexUvSet			= mc.mDisplacementTexUvSet;
					ext.mReflectionTexUvSet				= mc.mReflectionTexUvSet;
					ext.mLightmapTexUvSet				= mc.mLightmapTexUvSet;
					ext.mExtraTexUvSet					= mc.mExtraTexUvSet;

					ext.mDiffuseTexRotation				= mc.mDiffuseTexRotation;
					ext.mSpecularTexRotation			= mc.mSpecularTexRotation;
					ext.mAmbientTexRotation				= mc.mAmbientTexRotation;
					ext.mEmissiveTexRotation			= mc.mEmissiveTexRotation;
					ext.mHeightTexRotation				= mc.mHeightTexRotation;
					ext.mNormalsTexRotation				= mc.mNormalsTexRotation;
					ext.mShininessTexRotation			= mc.mShininessTexRotation;
					ext.mOpacityTexRotation				= mc.mOpacityTexRotation;
					ext.mDisplacementTexRotation		= mc.mDisplacementTexRotation;
					ext.mReflectionTexRotation			= mc.mReflectionTexRotation;
					ext.mLightmapTexRotation			= mc.mLightmapTexRotation;
					ext.mExtraTexRotation				= mc.mExtraTexRotation;
				}
			}
		}

		size_t numTexUsages = 0;
		for (const auto& entry : texNamesToBorderHandlingToUsages) {
			numTexUsages += entry.second.size();
		}

		size_t numWhiteTexUsages = whiteTexUsages.empty() ? 0 : 1;
		size_t numStraightUpNormalTexUsages = (straightUpNormalTexUsages.empty() ? 0 : 1);
		size_t numTexNamesToBorderHandlingToUsages = texNamesToBorderHandlingToUsages.size();
		auto numImageViews = numTexNamesToBorderHandlingToUsages + numWhiteTexUsages + numStraightUpNormalTexUsages;

		if (aSerializer) {
			aSerializer->get().archive(numWhiteTexUsages);
			aSerializer->get().archive(numStraightUpNormalTexUsages);
			aSerializer->get().archive(numImageViews);
			aSerializer->get().archive(numTexNamesToBorderHandlingToUsages);
		}

		const auto numSamplers = numTexUsages + numWhiteTexUsages + numStraightUpNormalTexUsages;
		std::vector<avk::image_sampler> imageSamplers;
		imageSamplers.reserve(numSamplers);

		// Create the white texture and assign its index to all usages
		if (numWhiteTexUsages > 0) {
			auto [tex, cmds] = create_1px_texture_cached({ 255, 255, 255, 255 }, avk::layout::shader_read_only_optimal, vk::Format::eR8G8B8A8Unorm, avk::memory_usage::device, aImageUsage, aSerializer);
			commandsToReturn.mNestedCommandsAndSyncInstructions.push_back(std::move(cmds));
			auto imgView = context().create_image_view(std::move(tex));
			avk::sampler smplr;
			if (aSerializer)
			{
				smplr = create_sampler_cached(aSerializer->get(), avk::filter_mode::nearest_neighbor, avk::border_handling_mode::repeat);
			}
			else
			{
				smplr = context().create_sampler(avk::filter_mode::nearest_neighbor, avk::border_handling_mode::repeat);
			}
			imageSamplers.push_back(context().create_image_sampler(std::move(imgView), std::move(smplr)));

			// Assign this image_sampler's index wherever it is referenced:
			if (!aSerializer ||
				(aSerializer && (aSerializer->get().mode() == serializer::mode::serialize))) {
				int index = static_cast<int>(imageSamplers.size() - 1);
				for (auto* img : whiteTexUsages) {
					*img = index;
				}
			}
		}

		// Create the normal texture, containing a normal pointing straight up, and assign to all usages
		if (numStraightUpNormalTexUsages > 0) {
			auto [tex, cmds] = create_1px_texture_cached({ 127, 127, 255, 0 }, avk::layout::shader_read_only_optimal, vk::Format::eR8G8B8A8Unorm, avk::memory_usage::device, aImageUsage, aSerializer);
			commandsToReturn.mNestedCommandsAndSyncInstructions.push_back(std::move(cmds));
			auto imgView = context().create_image_view(std::move(tex));
			avk::sampler smplr;
			if (aSerializer)
			{
				smplr = create_sampler_cached(aSerializer->get(), avk::filter_mode::nearest_neighbor, avk::border_handling_mode::repeat);
			}
			else
			{
				smplr = context().create_sampler(avk::filter_mode::nearest_neighbor, avk::border_handling_mode::repeat);
			}
			imageSamplers.push_back(context().create_image_sampler(std::move(imgView), std::move(smplr)));

			// Assign this image_sampler's index wherever it is referenced:
			if (!aSerializer ||
				(aSerializer && (aSerializer->get().mode() == serializer::mode::serialize))) {
				int index = static_cast<int>(imageSamplers.size() - 1);
				for (auto* img : straightUpNormalTexUsages) {
					*img = index;
				}
			}
		}

		// Load all the images from file, and assign them to all usages
		if (!aSerializer ||
			(aSerializer && (aSerializer->get().mode() == serializer::mode::serialize))) {
			for (auto& pair : texNamesToBorderHandlingToUsages) {
				assert(!pair.first.empty());

				bool potentiallySrgb = srgbTextures.contains(pair.first);

				// create_image_from_file_cached takes the serializer as an optional,
				// therefore the call is safe with and without one
				auto [tex, cmds] = create_image_from_file_cached(pair.first, true, potentiallySrgb, aFlipTextures, 4, avk::layout::shader_read_only_optimal, avk::memory_usage::device, aImageUsage, aSerializer);
				commandsToReturn.mNestedCommandsAndSyncInstructions.push_back(std::move(cmds));
				auto imgView = context().create_image_view(std::move(tex));
				assert(!pair.second.empty());

				// It is now possible that an image can be referenced from different samplers, which adds support for different
				// usages of an image, e.g. once it is used as a tiled texture, at a different place it is clamped to edge, etc.
				// If we are serializing, we need to store how many different samplers are referencing the image:
				auto numDifferentSamplers = static_cast<int>(pair.second.size());
				if (aSerializer) {
					aSerializer->get().archive(numDifferentSamplers);
				}

				// There can be different border handling types specified for the textures
				for (auto& [bhModes, usages] : pair.second) {
					assert(!usages.empty());
					
					avk::sampler smplr;
					if (aSerializer) {
						smplr = create_sampler_cached(aSerializer->get(), aTextureFilterMode, bhModes);
					}
					else
					{
						smplr = context().create_sampler(aTextureFilterMode, bhModes);
					}

					if (numDifferentSamplers > 1) {
						// If we indeed have different border handling modes, create multiple samplers and share the image view resource among them:
						imageSamplers.push_back(context().create_image_sampler(imgView, std::move(smplr)));
					}
					else {
						// There is only one border handling mode:
						imageSamplers.push_back(context().create_image_sampler(std::move(imgView), std::move(smplr)));
					}

					// Assign the texture usages:
					auto index = static_cast<int>(imageSamplers.size() - 1);
					for (auto* img : usages) {
						*img = index;
					}
				}
			}
		}
		else {
			// We sure have the serializer here
			for (int i = 0; i < numTexNamesToBorderHandlingToUsages; ++i) {
				// create_image_from_file_cached does not need these values, as the
				// image data is loaded from a cache file, so we can just pass some
				// defaults to avoid having to save them to the cache file
				const bool potentiallySrgbDontCare = false;
				const std::string pathDontCare = "";

				// Read an image from cache
				auto [tex, cmds] = create_image_from_file_cached(pathDontCare, true, potentiallySrgbDontCare, aFlipTextures, 4, avk::layout::shader_read_only_optimal, avk::memory_usage::device, aImageUsage, aSerializer);
				commandsToReturn.mNestedCommandsAndSyncInstructions.push_back(std::move(cmds));
				auto imgView = context().create_image_view(std::move(tex));

				// Read the number of samplers from cache
				int numDifferentSamplers;
				aSerializer->get().archive(numDifferentSamplers);

				// Initialize modes with some default values to make the compiler happy. We do not care which defaults are used,
				// since the actual modes are read from the cache file in create_sampler_cached.
				std::array<avk::border_handling_mode, 3> bhModes = { avk::border_handling_mode::clamp_to_edge, avk::border_handling_mode::clamp_to_edge, avk::border_handling_mode::clamp_to_edge };

				// There can be different border handling types specified for the textures
				for (int i = 0; i < numDifferentSamplers; ++i) {
					
					// Read sampler from cache
					auto smplr = create_sampler_cached(aSerializer->get(), aTextureFilterMode, bhModes);

					if (numDifferentSamplers > 1) {
						// If we indeed have different border handling modes, create multiple samplers and share the image view resource among them:
						imageSamplers.push_back(context().create_image_sampler(imgView, std::move(smplr)));
					}
					else {
						// There is only one border handling mode:
						imageSamplers.push_back(context().create_image_sampler(std::move(imgView), std::move(smplr)));
					}
				}
			}
		}

		if (aSerializer) {
			aSerializer->get().archive(result);
		}

		// Hand over ownership to the caller
		return std::make_tuple(std::move(result), std::move(imageSamplers), std::move(commandsToReturn));
	}

	
	/**	Convert the given material config into a format that is usable with a GPU buffer for the materials (i.e. properly vec4-aligned),
	 *	and a set of images and samplers, which are already created on and uploaded to the GPU.
	 *	@param	aSerializer					The serializer used to store the data to or load the data from a cache file, depending on its mode.
	 *	@param	aMaterialConfigs			The material config in CPU format
	 *	@param	aLoadTexturesInSrgb			Set to true to load the images in sRGB format if applicable
	 *	@param	aFlipTextures				Set to true to y-flip images			
	 *	@param	aImageUsage					How this image is going to be used. Can be a combination of different avk::image_usage values
	 *	@param	aTextureFilterMode			Texture filtering mode for all the textures. Trilinear or anisotropic filtering modes will trigger MIP-maps to be generated.
	 *	@return	A tuple of three elements:
	 *			<0>: A collection of structs that contains material data converted to a GPU-suitable format. Image indices refer to the indices of the second tuple element:
	 *			<1>: A list of image samplers that were loaded from the referenced images in aMaterialConfigs, i.e. these are already actual GPU resources.
	 *			<2>: Zero, one, or multiple commands that need to be executed to complete the operation. This can include operations like uploading image data, or creating mip maps.
	 */
	template <typename T>
	std::tuple<std::vector<T>, std::vector<avk::image_sampler>, avk::command::action_type_command> convert_for_gpu_usage_cached(
		avk::serializer& aSerializer,
		const std::vector<avk::material_config>& aMaterialConfigs,
		bool aLoadTexturesInSrgb = false,
		bool aFlipTextures = false,
		avk::image_usage aImageUsage = avk::image_usage::general_texture,
		avk::filter_mode aTextureFilterMode = avk::filter_mode::trilinear)
	{
		return convert_for_gpu_usage_cached<T>(
			aMaterialConfigs,
			aLoadTexturesInSrgb,
			aFlipTextures,
			aImageUsage,
			aTextureFilterMode,
			aSerializer);
	}

	/**	Takes a vector of avk::material_config elements and converts it into a format that is usable
	 *	in shaders. Concretely, this means that each input avk::material_config is transformed into
	 *	a avk::material_gpu_data struct. The latter no longer contains the paths to images, but
	 *	instead, indices to image samplers.
	 *	The image samplers referenced by those indices are returned as the second tuple element.
	 *
	 *	Whenever textures are not set in the input avk::material_config elements, they will be
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
	 *			auto e = imageSamplers[i]->get_image_view()->get_image().create_info().extent;
	 *			if (e.width == 1u && e.height == 1u) {
	 *				++numAutoGen;
	 *			}
	 *		}
	 *
	 *	@param	aMaterialConfigs		A vector of multiple avk::material_config entries that are to
	 *									be converted into vectors of avk::material_gpu_data and avk::image_sampler
	 *	@param	aLoadTexturesInSrgb		If true, "diffuse textures", "ambient textures", and "extra textures"
	 *									are assumed to be in sRGB format and will be loaded as such.
	 *									All other textures will always be loaded in non-sRGB format.
	 *	@param	aFlipTextures			Flip the images loaded from file vertically.
	 *	@param	aImageUsage				Image usage for all the textures that are loaded.
	 *	@param	aTextureFilterMode		Texture filter mode for all the textures that are loaded.
	 *	@param	aBorderHandlingMode		Border handling mode for all the textures that are loaded.
	 *	@return	A tuple of three elements:
	 *			<0>: A collection of structs that contains material data converted to a GPU-suitable format. Image indices refer to the indices of the second tuple element:
	 *			<1>: A list of image samplers that were loaded from the referenced images in aMaterialConfigs, i.e. these are already actual GPU resources.
	 *			<2>: Zero, one, or multiple commands that need to be executed to complete the operation. This can include operations like uploading image data, or creating mip maps.
	 */
	template <typename T>
	std::tuple<std::vector<T>, std::vector<avk::image_sampler>, avk::command::action_type_command> convert_for_gpu_usage(
		const std::vector<avk::material_config>& aMaterialConfigs,
		bool aLoadTexturesInSrgb = false,
		bool aFlipTextures = false,
		avk::image_usage aImageUsage = avk::image_usage::general_texture,
		avk::filter_mode aTextureFilterMode = avk::filter_mode::trilinear)
	{
		return convert_for_gpu_usage_cached<T>(
			aMaterialConfigs,
			aLoadTexturesInSrgb,
			aFlipTextures,
			aImageUsage,
			aTextureFilterMode);
	}
}
