
#include "image_data.hpp"
#include "vk_convenience_functions.hpp"

namespace avk
{	
	/** Implementation of image_data_implementor interface that represents cube map image data composed from individual image files
	*/
	class image_data_composite_cubemap : public image_data_implementor
	{
	public:
		explicit image_data_composite_cubemap(const std::vector<std::string>& aPaths, const bool aLoadHdrIfPossible = false, const bool aLoadSrgbIfApplicable = false, const bool aFlip = false, const int aPreferredNumberOfTextureComponents = 4)
			: image_data_implementor(aPaths, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents)
		{
			assert(aPaths.size() == 6);

			for (const auto& path : aPaths)
			{
				std::unique_ptr<image_data_implementor> i = load_image_data_from_file(path, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents);

				image_data_implementors.push_back(std::move(i));
			}

			assert(image_data_implementors.size() == 6);
		}

		void load()
		{
			for (auto& r : image_data_implementors)
			{
				r->load();

				assert(!r->empty());
				assert(r->layers() == 1);
				// target must be 2D
				assert(r->target() == vk::ImageType::e2D);
				// must not be a cube map
				assert(r->faces() == 1);

				if (!image_data_implementors.empty())
				{
					// all image resources must have the same format, target and extent
					auto& r0 = image_data_implementors[0];
					assert(r->get_format() == r0->get_format());
					assert(r->target() == r0->target());
					assert(r->extent() == r0->extent());
					// mipmap levels must agree
					assert(r->levels() == r0->levels());
				}
			}
		}

		bool empty() const
		{
			auto is_empty = false;

			for (const auto& r : image_data_implementors)
			{
				is_empty = is_empty || r->empty();
			}

			return is_empty;
		}

		vk::Format get_format() const
		{
			return image_data_implementors[0]->get_format();
		}

		vk::ImageType target() const
		{
			return image_data_implementors[0]->target();
		}

		extent_type extent(const uint32_t level = 0) const
		{
			return image_data_implementors[0]->extent(level);
		}

		void* get_data(const uint32_t layer, const uint32_t face, const uint32_t level)
		{
			return image_data_implementors[face]->get_data(0, 0, level);
		}

		size_t size() const
		{
			return image_data_implementors[0]->size();
		}

		size_t size(const uint32_t level) const
		{
			return image_data_implementors[0]->size(level);
		}

		uint32_t levels() const
		{
			return image_data_implementors[0]->levels();
		}

		uint32_t faces() const
		{
			return 6;
		}

		bool is_hdr() const
		{
			return image_data_implementors[0]->is_hdr();
		}

	private:
		std::vector<std::unique_ptr<image_data_implementor>> image_data_implementors;
	};

	/** Implementation of image_data_implementor interface for loading image files with the GLI image library
	*/
	class image_data_gli : public image_data_implementor
	{
	public:
		explicit image_data_gli(const std::string& aPath, const bool aLoadHdrIfPossible = false, const bool aLoadSrgbIfApplicable = false, const bool aFlip = false, const int aPreferredNumberOfTextureComponents = 4)
			: image_data_implementor(aPath, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents)
		{
		}

		void load()
		{
			gliTex = gli::load(path());

			if (!gliTex.empty() && mFlip)
			{
				flip();
			}
		}

		vk::Format get_format() const
		{
			// TODO: what if mLoadHdrIfPossible == false but file has a HDR format? Likewise, mLoadSrgbIfApplicable is not considered here.
			return map_format_gli_to_vk(gliTex.format());
		};

		vk::ImageType target() const
		{
			switch (gliTex.target())
			{
			case gli::TARGET_1D:
			case gli::TARGET_1D_ARRAY:
				return vk::ImageType::e1D;
			case gli::TARGET_2D:
			case gli::TARGET_2D_ARRAY:
			case gli::TARGET_CUBE:
			case gli::TARGET_CUBE_ARRAY:
				return vk::ImageType::e2D;
			case gli::TARGET_3D:
				return vk::ImageType::e3D;
			default:
				throw avk::runtime_error("Unknown target");
			}
		}

		extent_type extent(const uint32_t level = 0) const
		{
			auto e = gliTex.extent(level);

			return vk::Extent3D(e[0], e[1], e[2]);
		};

		void* get_data(const uint32_t layer, const uint32_t face, const uint32_t level)
		{
			return gliTex.data(layer, face, level);
		};

		size_t size() const
		{
			return gliTex.size();
		}

		size_t size(const uint32_t level) const
		{
			return gliTex.size(level);
		}

		uint32_t levels() const
		{
			return static_cast<uint32_t>(gliTex.levels());
		};

		uint32_t layers() const
		{
			return static_cast<uint32_t>(gliTex.layers());
		};

		uint32_t faces() const
		{
			return static_cast<uint32_t>(gliTex.faces());
		};

		bool can_flip() const
		{
			switch (gliTex.target())
			{
			case gli::TARGET_2D:
			case gli::TARGET_2D_ARRAY:
			case gli::TARGET_CUBE:
			case gli::TARGET_CUBE_ARRAY:
				return !gli::is_compressed(gliTex.format()) || gli::is_s3tc_compressed(gliTex.format());
			default:
				return false;
			}
		};

		bool empty() const
		{
			return gliTex.empty();
		};

	protected:
		void flip()
		{
			assert(can_flip());

			gliTex = gli::flip(gliTex);
		};

	private:
		/** Map GLI image format enum to Vulkan image format enum
		* @param aGliFmt	a valid GLI format value
		* @return the Vulkan image format enum value that corresponds to the input parameter
		*/
		static vk::Format map_format_gli_to_vk(const gli::format aGliFmt)
		{
			vk::Format imFmt;

			switch (aGliFmt) {
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
			default:
				imFmt = vk::Format::eUndefined;
			}

			return imFmt;
		}

		gli::texture gliTex;
	};

	/** Implementation of image_data_implementor interface for loading image files with the stbi image library
	*/
	class image_data_stb : public image_data_implementor
	{
	private:
		using type_8bit = stbi_uc;

	public:
		explicit image_data_stb(const std::string& aPath, const bool aLoadHdrIfPossible = false, const bool aLoadSrgbIfApplicable = false, const bool aFlip = false, const int aPreferredNumberOfTextureComponents = 4)
			: image_data_implementor(aPath, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents), mExtent(0, 0, 0), mChannelsInFile(0), sizeofPixelPerChannel(0), mFormat(vk::Format::eUndefined), mData(nullptr, &deleter)
		{
		}

		void load()
		{
			stbi_set_flip_vertically_on_load(mFlip);

			int w = 0, h = 0;

			mLoadHdrIfPossible = mLoadHdrIfPossible && stbi_is_hdr(path().c_str());

			// TODO: load 16 bit per channel files?
			if (mLoadHdrIfPossible)
			{
				void* data = stbi_loadf(path().c_str(), &w, &h, &mChannelsInFile, map_to_stbi_channels(mPreferredNumberOfTextureComponents));
				mData = std::unique_ptr<void, decltype(&deleter)>(data, &deleter);
				sizeofPixelPerChannel = sizeof(float);
			}
			else
			{
				void* data = stbi_load(path().c_str(), &w, &h, &mChannelsInFile, map_to_stbi_channels(mPreferredNumberOfTextureComponents));
				mData = std::unique_ptr<void, decltype(&deleter)>(data, &deleter);
				sizeofPixelPerChannel = sizeof(stbi_uc);
			}

			if (!mData) {
				LOG_INFO_EM(std::format("Result: {}", std::string(stbi_failure_reason())));
			
				throw avk::runtime_error(std::format("Couldn't load image from '{}' using stbi_load{}", path(), mLoadHdrIfPossible ? "f" : ""));
			}
			assert(mData != nullptr);

			mExtent = vk::Extent3D(w, h, 1);

			mFormat = select_format(mPreferredNumberOfTextureComponents, mLoadHdrIfPossible, mLoadSrgbIfApplicable);
		};

		vk::Format get_format() const
		{
			return mFormat;
		};

		vk::ImageType target() const
		{
			// stb_image only supports 2D texture targets
			return vk::ImageType::e2D;
		}

		extent_type extent(const uint32_t level = 0) const
		{
			// stb_image does not support mipmap levels
			assert(level == 0);

			return mExtent;
		};

		void* get_data(const uint32_t layer, const uint32_t face, const uint32_t level)
		{
			// stb_image does not support layers, faces or levels
			assert(layer == 0 && face == 0 && level == 0);

			return mData.get();
		};

		size_t size() const
		{
			return mExtent.width * mExtent.height * mPreferredNumberOfTextureComponents * sizeofPixelPerChannel;
		}

		size_t size(const uint32_t level) const
		{
			assert(level == 0);

			return size();
		}

		bool empty() const
		{
			return mData == nullptr;
		};

	private:
		/** Map preferred number of texture components to stbi channels
		* @param aPreferredNumberOfTextureComponents the preferred number of texture components, must be 1, 2, 3, or 4. 
		* @return the stbi internal constant that represents the given number of texture components
		*/
		static int map_to_stbi_channels(const int aPreferredNumberOfTextureComponents)
		{
			// TODO: autodetect number of channels in file (= 0)?
			assert(0 < aPreferredNumberOfTextureComponents && aPreferredNumberOfTextureComponents <= 4);

			switch (aPreferredNumberOfTextureComponents)
			{
			case 1:
				return STBI_grey;
			case 2:
				return STBI_grey_alpha;
			case 3:
				return STBI_rgb;
			case 4:
			default:
				return STBI_rgb_alpha;

			}
		}

		/** Select Vulkan image format based on preferred number of texture components, HDR, and sRGB requirements
		* @param aPreferredNumberOfTextureComponents the preferred number of texture components, must be 1, 2, 3, or 4. 
		* @param aLoadHdrIfPossible		load the texture as HDR (high dynamic range) data, if supported by the image loading library. If set to true, the image data may be returned in a HDR format even if the texture file does not contain HDR data. If set to false, the image data may be returned in an LDR format even if the texture contains HDR data. It is therefore advised to set this parameter according to the data format of the texture file.
		* @param aLoadSrgbIfApplicable	load the texture as sRGB color-corrected data, if supported by the image loading library. If set to true, the image data may be returned in an sRGB format even if the texture file does not contain sRGB data. If set to false, the image data may be returned in a plain RGB format even if the texture contains sRGB data. It is therefore advised to set this parameter according to the color space of the texture file.
		* @return the stbi internal constant that represents the given number of texture components
		*/
		static vk::Format select_format(const int aPreferredNumberOfTextureComponents, const bool aLoadHdrIfPossible, const bool aLoadSrgbIfApplicable)
		{
			vk::Format imFmt = vk::Format::eUndefined;

			if (aLoadHdrIfPossible) {
				switch (aPreferredNumberOfTextureComponents) {
				case 4:
				default:
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
				}
				//}
			}
			else if (aLoadSrgbIfApplicable) {
				switch (aPreferredNumberOfTextureComponents) {
				case 4:
				default:
					imFmt = avk::default_srgb_4comp_format();
					break;
					// Attention: There's a high likelihood that your GPU does not support formats with less than four color components!
				case 3:
					imFmt = avk::default_srgb_3comp_format();
					break;
				case 2:
					imFmt = avk::default_srgb_2comp_format();
					break;
				case 1:
					imFmt = avk::default_srgb_1comp_format();
					break;
				}
			}
			else {
				switch (aPreferredNumberOfTextureComponents) {
				case 4:
				default:
					imFmt = avk::default_rgb8_4comp_format();
					break;
					// Attention: There's a high likelihood that your GPU does not support formats with less than four color components!
				case 3:
					imFmt = avk::default_rgb8_3comp_format();
					break;
				case 2:
					imFmt = avk::default_rgb8_2comp_format();
					break;
				case 1:
					imFmt = avk::default_rgb8_1comp_format();
					break;
				}
			}

			return imFmt;
		}

		// stb_image uses malloc() for memory allocation, therefore it should be deallocated with free()
		static void deleter(void* data) {
			stbi_image_free(data);
		};

		vk::Extent3D mExtent;
		int mChannelsInFile;
		size_t sizeofPixelPerChannel;
		vk::Format mFormat;

		std::unique_ptr<void, decltype(&deleter)> mData;
	};

	std::unique_ptr<image_data_implementor> image_data_interface::load_image_data_from_file(const std::string& aPath, const bool aLoadHdrIfPossible, const bool aLoadSrgbIfApplicable, const bool aFlip, const int aPreferredNumberOfTextureComponents)
	{
		// try loading with GLI
		std::unique_ptr<image_data_implementor> retval(new image_data_gli(aPath, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents));
		retval->load();

		// try loading with stb
		if (retval->empty())
		{
			retval = std::unique_ptr<image_data_implementor>(new image_data_stb(aPath, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents));
			retval->load();
		}

		if (retval->empty())
		{
			throw avk::runtime_error(std::format("Could not load image from '{}'", aPath));
		}

		return retval;
	}

	std::unique_ptr<image_data_implementor> image_data_interface::load_image_data_from_file(const std::vector<std::string>& aPaths, const bool aLoadHdrIfPossible, const bool aLoadSrgbIfApplicable, const bool aFlip, const int aPreferredNumberOfTextureComponents)
	{
		std::unique_ptr<image_data_implementor> retval(new image_data_composite_cubemap(aPaths, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents));
		retval->load();

		return retval;
	}
	
}
