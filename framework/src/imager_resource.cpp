#include <gvk.hpp>
#include "image_resource.hpp"

namespace gvk
{
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

	class image_resource_gli_t : public image_resource_t
	{
	public:
		image_resource_gli_t(const std::string& aPath, bool aHDR = false, bool asRGB = false, bool aFlip = false, int aPreferredNumberOfTextureComponents = 4)
			: image_resource_t(aPath, aHDR, asRGB, aFlip, aPreferredNumberOfTextureComponents), gliTex(gli::load(aPath))
		{
			if (aFlip)
			{
				flip();
			}
		};

		~image_resource_gli_t() {}

		vk::Format get_format()
		{
			// TODO: what if mHDR == false but file has a HDR format? Likewise, msRGB is not considered here.
			return map_format_gli_to_vk(gliTex.format()).value();
		};

		vk::ImageType target()
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
				throw std::exception("Unknown target");
			}
		}

		extent_type extent()
		{
			auto e = gliTex.extent();

			return vk::Extent3D(e[0], e[1], e[2]);
		};

		extent_type extent(size_t level)
		{
			auto e = gliTex.extent(level);

			return vk::Extent3D(e[0], e[1], e[2]);
		};

		void* get_data(size_t layer, size_t face, size_t level)
		{
			return gliTex.data(layer, face, level);
		};

		size_t size(size_t level)
		{
			return gliTex.size(level);
		}

		// Mipmap levels; 1 if no Mipmapping, 0 if Mipmaps should be created after loading?
		size_t levels()
		{
			return gliTex.levels();
		};
		// array layers, for texture arrays

		size_t layers()
		{
			return gliTex.layers();
		};

		// faces in cubemap
		size_t faces()
		{
			return gliTex.faces();
		};

		void flip()
		{
			assert(can_flip());

			gliTex = gli::flip(gliTex);
		};

		bool can_flip()
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

		bool empty()
		{
			return gliTex.empty();
		};

	private:
		//avk::Format map_format_gli_to_vk(const gli::format gliFmt);

		gli::texture gliTex;
	};

	class image_resource_stb_t : public image_resource_t
	{
		typedef stbi_uc type_8bit;

	public:
		image_resource_stb_t(const std::string& aPath, bool aHDR = false, bool asRGB = false, bool aFlip = false, int aPreferredNumberOfTextureComponents = 4)
			: image_resource_t(aPath, aHDR, asRGB, aFlip, aPreferredNumberOfTextureComponents), mExtent(0, 0, 0), mChannelsInFile(0), mData(nullptr, &deleter)
		{
			stbi_set_flip_vertically_on_load(aFlip);

			int w = 0, h = 0;

			LOG_INFO_EM(fmt::format("Loading file '{}' with stb...", aPath));

			mHDR = mHDR && stbi_is_hdr(aPath.c_str());

			// TODO: load 16 bit per channel files?
			if (mHDR)
			{
				void* data = stbi_loadf(mPath.c_str(), &w, &h, &mChannelsInFile, map_to_stbi_channels(mPreferredNumberOfTextureComponents));
				mData = std::unique_ptr<void, decltype(&deleter)>(data, &deleter);
				sizeofPixelPerChannel = sizeof(float);
			}
			else
			{
				void* data = stbi_load(mPath.c_str(), &w, &h, &mChannelsInFile, map_to_stbi_channels(mPreferredNumberOfTextureComponents));
				mData = std::unique_ptr<void, decltype(&deleter)>(data, &deleter);
				sizeofPixelPerChannel = sizeof(stbi_uc);
			}

			//LOG_INFO_EM(fmt::format("Result: {}", std::string(stbi_failure_reason())));
			
			/*if (!mData) {
				throw gvk::runtime_error(fmt::format("Couldn't load image from '{}' using stbi_load{}", aPath, mHDR ? "f" : ""));
			}*/
			//assert(mData != nullptr);

			mExtent = vk::Extent3D(w, h, 1);

			mFormat = select_format(mPreferredNumberOfTextureComponents, mHDR, msRGB);
		};

		~image_resource_stb_t() 
		{
		}

		vk::Format get_format()
		{
			return mFormat;
		};

		vk::ImageType target()
		{
			// stb_image only supports 2D texture targets
			return vk::ImageType::e2D;
		}

		extent_type extent()
		{
			return mExtent;
		};

		extent_type extent(size_t level)
		{
			// stb_image does not support mipmap levels
			assert(level == 0);

			return mExtent;
		};

		void* get_data(size_t layer, size_t face, size_t level)
		{
			// stb_image does not support layers, faces or levels
			assert(layer == 0 && face == 0 && level == 0);

			return mData.get();
		};

		size_t size(size_t level)
		{
			assert(level == 0);

			return mExtent.width * mExtent.height * mPreferredNumberOfTextureComponents * sizeofPixelPerChannel;
		}

		bool empty()
		{
			return mData == nullptr;
		};

	private:
		static int map_to_stbi_channels(int aPreferredNumberOfTextureComponents)
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

		static vk::Format select_format(int aPreferredNumberOfTextureComponents, bool aLoadHdr, bool aLoadSrgb)
		{
			vk::Format imFmt = vk::Format::eUndefined;

			if (aLoadHdr) {
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
			else if (aLoadSrgb) {
				switch (aPreferredNumberOfTextureComponents) {
				case 4:
				default:
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
				}
			}
			else {
				switch (aPreferredNumberOfTextureComponents) {
				case 4:
				default:
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
				}
			}

			return imFmt;
		}

		// stb_image uses malloc() for memory allocation, therefore it should be deallocated with free()
		static void deleter(void* data) {
			stbi_image_free(data);
		};

		vk::Extent3D mExtent;
		//size_t mWidth, mHeight;
		int mChannelsInFile;
		size_t sizeofPixelPerChannel;
		vk::Format mFormat;

		std::unique_ptr<void, decltype(&deleter)> mData;

		//avk::Format map_format_gli_to_vk(const gli::format gliFmt);
	};

	image_resource image_resource_t::create_image_resource_from_file(const std::string& aPath, bool aLoadHdrIfPossible, bool aLoadSrgbIfApplicable, bool aFlip,
		int aPreferredNumberOfTextureComponents)
	{
		image_resource retval(new image_resource_gli_t(aPath, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents));

		if (retval->empty())
		{
			retval = image_resource(new image_resource_stb_t(aPath, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents));
		}

		if (retval->empty())
		{
			throw gvk::runtime_error(fmt::format("Could not load image from '{}'", aPath));
		}

		return retval;
	}

	image_resource image_resource_t::create_image_resource_from_file(const std::vector<std::string>& aPaths, bool aLoadHdrIfPossible, bool aLoadSrgbIfApplicable, bool aFlip,
		int aPreferredNumberOfTextureComponents)
	{
		image_resource retval(new composite_cubemap_image_resource_t(aPaths, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents));

		return retval;
	}
}