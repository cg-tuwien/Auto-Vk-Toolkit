#pragma once
#include <gvk.hpp>

namespace gvk
{
	class image_resource_t;
	class image_resource_impl_t;

	typedef std::unique_ptr<image_resource_t> image_resource;

	// interface of image_resource type, used for abstraction and implementor in bridge pattern
	class image_resource_base_t
	{
	public:
		// type that represents the size of 1D, 2D, and 3D images
		typedef vk::Extent3D extent_type;

		// load image resource into memory
		// perform this as an extra step to facilitate caching of image resources
		virtual void load() = 0;

		virtual vk::Format get_format() const = 0;

		virtual vk::ImageType target() const = 0;

		// size of image resource, in pixels
		virtual extent_type extent(uint32_t level = 0) const = 0;

		// Note: the return type cannot be const void* because this would result in a compile-time error with the deserializer;
		// The function cannot be const either or gli would call a function overload that returns a const void*
		virtual void* get_data(uint32_t layer, uint32_t face, uint32_t level) = 0;
		// size of whole image resource, in bytes
		virtual size_t size() const = 0;
		// size of one mipmap leve of image resource, in bytes
		virtual size_t size(uint32_t level) const = 0;

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
		
		image_resource_base_t(const std::string& aPath, bool aHDR = false, bool asRGB = false, bool aFlip = false, int aPreferredNumberOfTextureComponents = 4)
			: mPaths({ aPath }), mHDR(aHDR), msRGB(asRGB), mFlip(aFlip), mPreferredNumberOfTextureComponents(aPreferredNumberOfTextureComponents)
		{
		}
		
		// for cubemaps loaded from six individual images
		image_resource_base_t(const std::vector<std::string>& aPaths, bool aHdr = false, bool aSrgb = false, bool aFlip = false, int aPreferredNumberOfTextureComponents = 4)
			: mPaths(aPaths), mHDR(aHDR), msRGB(asRGB), mFlip(aFlip), mPreferredNumberOfTextureComponents(aPreferredNumberOfTextureComponents)
		{
		}

		// Note that boolean flags are not applicable in all cases; stb_image converts image data to or from float format, but other image loading libraries, like GLI, do not;
		// Likewise, sRGB is ignored for GLI loader, and it may not be possible to flip images on loading (e.g. compressed textures)
		static std::unique_ptr<image_resource_impl_t> load_image_resource_from_file(const std::string& aPath, bool aLoadHdrIfPossible = true, bool aLoadSrgbIfApplicable = true, bool aFlip = true,
			int aPreferredNumberOfTextureComponents = 4);
		
		// for cubemaps loaded from six individual files
		// Order of faces +X, -X, +Y, -Y, +Z, -Z
		static std::unique_ptr<image_resource_impl_t> load_image_resource_from_file(const std::vector<std::string>& aPaths, bool aLoadHdrIfPossible = true, bool aLoadSrgbIfApplicable = true, bool aFlip = true, int aPreferredNumberOfTextureComponents = 4);
		
		std::vector<std::string> mPaths;

		// if image should be flipped vertically when loaded, if possible
		bool mFlip;
		bool mHdr;
		bool mSrgb;
		int mPreferredNumberOfTextureComponents;
	};

	// base class of implementor of image_resource type in bridge pattern
	class image_resource_impl_t : public image_resource_base_t
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
		image_resource_impl_t(const std::string& aPath, bool aHDR = false, bool asRGB = false, bool aFlip = false, int aPreferredNumberOfTextureComponents = 4)
			: image_resource_base_t(aPath, aHDR, asRGB, aFlip, aPreferredNumberOfTextureComponents)
		{
		}

		image_resource_impl_t(const std::vector<std::string>& aPaths, bool aHDR = false, bool asRGB = false, bool aFlip = false, int aPreferredNumberOfTextureComponents = 4)
			: image_resource_base_t(aPaths, aHDR, asRGB, aFlip, aPreferredNumberOfTextureComponents)
		{
		}
	};

	// base class of abstraction in bridge pattern
	class image_resource_t : public image_resource_base_t
	{
	public:
		image_resource_t(const std::string& aPath, bool aHDR = false, bool asRGB = false, bool aFlip = false, int aPreferredNumberOfTextureComponents = 4)
			: image_resource_base_t(aPath, aHDR, asRGB, aFlip, aPreferredNumberOfTextureComponents), pimpl(nullptr)
		{
		}

		image_resource_t(const std::vector<std::string>& aPaths, bool aHDR = false, bool asRGB = false, bool aFlip = false, int aPreferredNumberOfTextureComponents = 4)
			: image_resource_base_t(aPaths, aHDR, asRGB, aFlip, aPreferredNumberOfTextureComponents), pimpl(nullptr)
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
				pimpl = load_image_resource_from_file(mPaths[0], mHDR, msRGB, mFlip, mPreferredNumberOfTextureComponents);
			}
			else
			{
				pimpl = load_image_resource_from_file(mPaths, mHDR, msRGB, mFlip, mPreferredNumberOfTextureComponents);
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

		virtual extent_type extent(uint32_t level = 0) const
		{
			assert(!empty());
			assert(level < pimpl->levels());

			return pimpl->extent(level);
		}

		virtual void* get_data(uint32_t layer, uint32_t face, uint32_t level)
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

		virtual size_t size(uint32_t level) const
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
		std::unique_ptr<image_resource_impl_t> pimpl;
	};
}
