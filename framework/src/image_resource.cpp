#include <gvk.hpp>
#include "image_resource.hpp"

namespace gvk
{
	// implementation in bridge pattern
	class composite_cubemap_image_resource_t : public image_resource_impl_t
	{
	public:
		composite_cubemap_image_resource_t(const std::vector<std::string>& aPaths, bool aHDR = false, bool asRGB = false, bool aFlip = false, int aPreferredNumberOfTextureComponents = 4)
			: image_resource_impl_t(aPaths, aHDR, asRGB, aFlip, aPreferredNumberOfTextureComponents)
		{
			assert(aPaths.size() == 6);

			for (auto path : aPaths)
			{
				std::unique_ptr<image_resource_impl_t> i = load_image_resource_from_file(path, aHDR, asRGB, aFlip, aPreferredNumberOfTextureComponents);

				image_resources.push_back(std::move(i));
			}

			assert(image_resources.size() == 6);
		}

		void load()
		{
			for (auto& r : image_resources)
			{
				r->load();

				assert(!r->empty());
				assert(r->layers() == 1);
				// target must be 2D
				assert(r->target() == vk::ImageType::e2D);
				// must not be a cubemap
				assert(r->faces() == 1);

				if (image_resources.size() > 0)
				{
					// all image resources must have the same format, target and extent
					std::unique_ptr<image_resource_impl_t>& r0 = image_resources[0];
					assert(r->get_format() == r0->get_format());
					assert(r->target() == r0->target());
					assert(r->extent() == r0->extent());
					// Mipmap levels must agree
					assert(r->levels() == r0->levels());
				}
			}
		}

		bool empty() const
		{
			bool is_empty = false;

			for (auto& r : image_resources)
			{
				is_empty = is_empty || r->empty();
			}

			return is_empty;
		}

		vk::Format get_format() const
		{
			return image_resources[0]->get_format();
		}

		vk::ImageType target() const
		{
			return image_resources[0]->target();
		}

		extent_type extent(uint32_t level = 0) const
		{
			return image_resources[0]->extent(level);
		}

		void* get_data(uint32_t layer, uint32_t face, uint32_t level)
		{
			return image_resources[face]->get_data(0, 0, level);
		}

		size_t size() const
		{
			return image_resources[0]->size();
		}

		size_t size(uint32_t level) const
		{
			return image_resources[0]->size(level);
		}

		uint32_t levels() const
		{
			return image_resources[0]->levels();
		}

		uint32_t faces() const
		{
			return 6;
		}

		bool is_hdr() const
		{
			return image_resources[0]->is_hdr();
		}

	private:
		std::vector<std::unique_ptr<image_resource_impl_t>> image_resources;
	};

	class image_resource_gli_t : public image_resource_impl_t
	{
	public:
		image_resource_gli_t(const std::string& aPath, bool aHDR = false, bool asRGB = false, bool aFlip = false, int aPreferredNumberOfTextureComponents = 4)
			: image_resource_impl_t(aPath, aHDR, asRGB, aFlip, aPreferredNumberOfTextureComponents)
		{
		}

		void load()
		{
			gliTex = gli::load(path());

			if (mFlip)
			{
				flip();
			}
		}

		vk::Format get_format() const
		{
			// TODO: what if mHDR == false but file has a HDR format? Likewise, msRGB is not considered here.
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
				throw std::exception("Unknown target");
			}
		}

		extent_type extent(uint32_t level = 0) const
		{
			auto e = gliTex.extent(level);

			return vk::Extent3D(e[0], e[1], e[2]);
		};

		void* get_data(uint32_t layer, uint32_t face, uint32_t level)
		{
			return gliTex.data(layer, face, level);
		};

		size_t size() const
		{
			return gliTex.size();
		}

		size_t size(uint32_t level) const
		{
			return gliTex.size(level);
		}

		// Mipmap levels; 1 if no Mipmapping, 0 if Mipmaps should be created after loading
		uint32_t levels() const
		{
			// Vulkan uses uint32_t to store level and layer sizes
			return static_cast<uint32_t>(gliTex.levels());
		};

		// array layers, for texture arrays
		uint32_t layers() const
		{
			return static_cast<uint32_t>(gliTex.layers());
		};

		// faces in cubemap
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
		static vk::Format map_format_gli_to_vk(const gli::format gliFmt)
		{
			vk::Format imFmt = vk::Format::eUndefined;

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

		gli::texture gliTex;
	};

	class image_resource_stb_t : public image_resource_impl_t
	{
	private:
		typedef stbi_uc type_8bit;

	public:
		image_resource_stb_t(const std::string& aPath, bool aHDR = false, bool asRGB = false, bool aFlip = false, int aPreferredNumberOfTextureComponents = 4)
			: image_resource_impl_t(aPath, aHDR, asRGB, aFlip, aPreferredNumberOfTextureComponents), mExtent(0, 0, 0), mChannelsInFile(0), mData(nullptr, &deleter)
		{
		}

		void load()
		{
			stbi_set_flip_vertically_on_load(mFlip);

			int w = 0, h = 0;

			LOG_INFO_EM(fmt::format("Loading file '{}' with stb...", path()));

			mHDR = mHDR && stbi_is_hdr(path().c_str());

			// TODO: load 16 bit per channel files?
			if (mHDR)
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

			LOG_INFO_EM(fmt::format("Result: {}", std::string(stbi_failure_reason())));
			
			if (!mData) {
				throw gvk::runtime_error(fmt::format("Couldn't load image from '{}' using stbi_load{}", path(), mHDR ? "f" : ""));
			}
			assert(mData != nullptr);

			mExtent = vk::Extent3D(w, h, 1);

			mFormat = select_format(mPreferredNumberOfTextureComponents, mHDR, msRGB);
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

		extent_type extent(uint32_t level = 0) const
		{
			// stb_image does not support mipmap levels
			assert(level == 0);

			return mExtent;
		};

		void* get_data(uint32_t layer, uint32_t face, uint32_t level)
		{
			// stb_image does not support layers, faces or levels
			assert(layer == 0 && face == 0 && level == 0);

			return mData.get();
		};

		size_t size() const
		{
			return mExtent.width * mExtent.height * mPreferredNumberOfTextureComponents * sizeofPixelPerChannel;
		}

		size_t size(uint32_t level) const
		{
			assert(level == 0);

			return size();
		}

		bool empty() const
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
		int mChannelsInFile;
		size_t sizeofPixelPerChannel;
		vk::Format mFormat;

		std::unique_ptr<void, decltype(&deleter)> mData;
	};

	std::unique_ptr<image_resource_impl_t> image_resource_base_t::load_image_resource_from_file(const std::string& aPath, bool aLoadHdrIfPossible, bool aLoadSrgbIfApplicable, bool aFlip,
		int aPreferredNumberOfTextureComponents)
	{
		// try loading with GLI
		std::unique_ptr<image_resource_impl_t> retval(new image_resource_gli_t(aPath, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents));
		retval->load();

		// try loading with stb
		if (retval->empty())
		{
			retval = std::unique_ptr<image_resource_impl_t>(new image_resource_stb_t(aPath, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents));
			retval->load();
		}

		if (retval->empty())
		{
			throw gvk::runtime_error(fmt::format("Could not load image from '{}'", aPath));
		}

		return retval;
	}

	std::unique_ptr<image_resource_impl_t> image_resource_base_t::load_image_resource_from_file(const std::vector<std::string>& aPaths, bool aLoadHdrIfPossible, bool aLoadSrgbIfApplicable, bool aFlip, int aPreferredNumberOfTextureComponents)
	{
		std::unique_ptr<image_resource_impl_t> retval(new composite_cubemap_image_resource_t(aPaths, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents));
		retval->load();

		return retval;
	}
	
}