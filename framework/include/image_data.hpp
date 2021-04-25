#pragma once
#include <gvk.hpp>

namespace gvk
{
	class image_data;
	class image_data_implementor;


	// interface of image_data type, used for abstraction and implementor in bridge pattern
	class image_data_interface
	{
	public:
		// type that represents the size of 1D, 2D, and 3D images
		typedef vk::Extent3D extent_type;

		virtual ~image_data_interface() = default;
	    image_data_interface(image_data_interface&&) noexcept = default;
	    image_data_interface(const image_data_interface&) = delete;
	    image_data_interface& operator=(image_data_interface&&) noexcept = default;
	    image_data_interface& operator=(const image_data_interface&) = delete;
		
		// load image resource into memory
		// perform this as an extra step to facilitate caching of image resources
		virtual void load() = 0;

		virtual vk::Format get_format() const = 0;

		virtual vk::ImageType target() const = 0;

		// size of image resource, in pixels
		virtual extent_type extent(const uint32_t level = 0) const = 0;

		// Note: the return type cannot be const void* because this would result in a compile-time error with the deserializer;
		// The function cannot be const either or gli would call a function overload that returns a const void*
		virtual void* get_data(const uint32_t layer, const uint32_t face, const uint32_t level) = 0;
		// size of whole image resource, in bytes
		virtual size_t size() const = 0;
		// size of one mipmap level of image resource, in bytes
		virtual size_t size(const uint32_t level) const = 0;

		virtual bool empty() const = 0;

		// Vulkan uses uint32_t type for levels and layers (faces)
		virtual uint32_t levels() const = 0;

		// array layers, for texture arrays
		virtual uint32_t layers() const = 0;

		// faces in image, must be 6 for cubemaps, 1 for anything else
		virtual uint32_t faces() const = 0;

		// returns true if image format and image library support y-flipping when loading
		// TODO: make protected or remove?
		virtual bool can_flip() const = 0;

		// returns if image resource is a hdr format
		virtual bool is_hdr() const = 0;

		std::string path() const
		{
			return mPaths[0];
		}

		std::vector<std::string> paths() const
		{
			return mPaths;
		}

	protected:
		
		image_data_interface(const std::string& aPath, const bool aLoadHdrIfPossible = false, const bool aLoadSrgbIfApplicable = false, const bool aFlip = false, const int aPreferredNumberOfTextureComponents = 4)
			: mPaths({ aPath }), mLoadHdrIfPossible(aLoadHdrIfPossible), mLoadSrgbIfApplicable(aLoadSrgbIfApplicable), mFlip(aFlip), mPreferredNumberOfTextureComponents(aPreferredNumberOfTextureComponents)
		{
		}
		
		// for cubemaps loaded from six individual images
		image_data_interface(const std::vector<std::string>& aPaths, const bool aLoadHdrIfPossible = false, const bool aLoadSrgbIfApplicable = false, const bool aFlip = false, const int aPreferredNumberOfTextureComponents = 4)
			: mPaths(aPaths), mLoadHdrIfPossible(aLoadHdrIfPossible), mLoadSrgbIfApplicable(aLoadSrgbIfApplicable), mFlip(aFlip), mPreferredNumberOfTextureComponents(aPreferredNumberOfTextureComponents)
		{
		}

		// Note that boolean flags are not applicable in all cases; stb_image converts image data to or from float format, but other image loading libraries, like GLI, do not;
		// Likewise, sRGB is ignored for GLI loader, and it may not be possible to flip images on loading (e.g. compressed textures)
		static std::unique_ptr<image_data_implementor> load_image_data_from_file(const std::string& aPath, const bool aLoadHdrIfPossible = true, const bool aLoadSrgbIfApplicable = true, const bool aFlip = true, const int aPreferredNumberOfTextureComponents = 4);
		
		// for cubemaps loaded from six individual files
		// Order of faces +X, -X, +Y, -Y, +Z, -Z
		static std::unique_ptr<image_data_implementor> load_image_data_from_file(const std::vector<std::string>& aPaths, const bool aLoadHdrIfPossible = true, const bool aLoadSrgbIfApplicable = true, const bool aFlip = true, const int aPreferredNumberOfTextureComponents = 4);
		
		std::vector<std::string> mPaths;

		bool mLoadHdrIfPossible;
		bool mLoadSrgbIfApplicable;
		// if image should be flipped vertically when loaded, if possible
		bool mFlip;
		int mPreferredNumberOfTextureComponents;
	};

	// base class of implementor of image_data type in bridge pattern
	class image_data_implementor : public image_data_interface
	{
	public:
		// Default implementations for subclasses that don't support these optional features

		// Mipmap levels; 1 if no Mipmapping, 0 if Mipmaps should be created after loading
		virtual uint32_t levels() const 
		{
			return 1;
		}

		// array layers, for texture arrays
		virtual uint32_t layers() const
		{
			return 1;
		}

		// faces in cubemap, must be 6 for cubemaps, 1 for anything else
		virtual uint32_t faces() const
		{
			return 1;
		}

		// TODO: make protected or remove?
		virtual bool can_flip() const
		{
			return false;
		}

		virtual bool is_hdr() const
		{
			return false;
		}

	protected:
		explicit image_data_implementor(const std::string& aPath, const bool aLoadHdrIfPossible = false, const bool aLoadSrgbIfApplicable = false, const bool aFlip = false, const int aPreferredNumberOfTextureComponents = 4)
			: image_data_interface(aPath, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents)
		{
		}

		explicit image_data_implementor(const std::vector<std::string>& aPaths, const bool aLoadHdrIfPossible = false, const bool aLoadSrgbIfApplicable = false, const bool aFlip = false, const int aPreferredNumberOfTextureComponents = 4)
			: image_data_interface(aPaths, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents)
		{
		}
	};

	// base class of abstraction in bridge pattern
	class image_data : public image_data_interface
	{
	public:
		explicit image_data(const std::string& aPath, const bool aLoadHdrIfPossible = false, const bool aLoadSrgbIfApplicable = false, const bool aFlip = false, const int aPreferredNumberOfTextureComponents = 4)
			: image_data_interface(aPath, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents), pimpl(nullptr)
		{
		}

		explicit image_data(const std::vector<std::string>& aPaths, const bool aLoadHdrIfPossible = false, const bool aLoadSrgbIfApplicable = false, const bool aFlip = false, const int aPreferredNumberOfTextureComponents = 4)
			: image_data_interface(aPaths, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents), pimpl(nullptr)
		{
		}

		virtual void load()
		{
			if (!empty())
			{
				return;
			}

			assert(mPaths.size() == 1 || mPaths.size() == 6);

			if (mPaths.size() == 1)
			{
				pimpl = load_image_data_from_file(mPaths[0], mLoadHdrIfPossible, mLoadSrgbIfApplicable, mFlip, mPreferredNumberOfTextureComponents);
			}
			else
			{
				pimpl = load_image_data_from_file(mPaths, mLoadHdrIfPossible, mLoadSrgbIfApplicable, mFlip, mPreferredNumberOfTextureComponents);
			}

			pimpl->load();
		}

		virtual vk::Format get_format() const
		{
			assert(!empty());

			return pimpl->get_format();
		}

		virtual vk::ImageType target() const
		{
			assert(!empty());

			return pimpl->target();
		}

		virtual extent_type extent(const uint32_t level = 0) const
		{
			assert(!empty());
			assert(level < pimpl->levels());

			return pimpl->extent(level);
		}

		virtual void* get_data(const uint32_t layer, const uint32_t face, const uint32_t level)
		{
			assert(!empty());
			assert(layer < pimpl->layers());
			assert(face < pimpl->faces());
			assert(level < pimpl->levels());

			return pimpl->get_data(layer, face, level);
		}

		virtual size_t size() const
		{
			assert(!empty());

			return pimpl->size();
		}

		virtual size_t size(const uint32_t level) const
		{
			assert(!empty());
			assert(level < pimpl->levels());

			return pimpl->size(level);
		}

		virtual uint32_t levels() const
		{
			assert(!empty());

			return pimpl->levels();
		}

		// array layers, for texture arrays
		virtual uint32_t layers() const
		{
			assert(!empty());

			return pimpl->layers();
		}

		// faces in image, must be 6 for cubemaps, 1 for anything else
		virtual uint32_t faces() const
		{
			assert(!empty());

			return pimpl->faces();
		}

		// TODO: make protected or remove?
		virtual bool can_flip() const
		{
			assert(!empty());

			return pimpl->can_flip();
		}

		virtual bool is_hdr() const
		{
			assert(!empty());

			return pimpl->is_hdr();
		}

		virtual bool empty() const
		{
			return !(pimpl && !pimpl->empty());
		}

	private:
		// pimpl idiom: use unique_ptr
		// allocate pimpl in out-of-line constructor
		// deallocate in out-of-line destructor (complete type only known after class definition)
		// for user-defined destructor, there is no compiler-generated copy constructor and move-assignment operator; define out-of-line if needed

		// pointer-to-implementation
		std::unique_ptr<image_data_implementor> pimpl;
	};
}
