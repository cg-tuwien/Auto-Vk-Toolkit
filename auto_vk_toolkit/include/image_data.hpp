#pragma once

namespace avk
{
	class image_data;
	class image_data_implementor;


	/** Interface of image_data type, used for abstraction and implementor in bridge pattern
	* This class should only be derived by the image_data and image_data_implementor classes
	*/
	class image_data_interface
	{
	public:
		/** type that represents the size of 1D, 2D, and 3D images
		*/
		using extent_type = vk::Extent3D;

		virtual ~image_data_interface() = default;
		/** Move constructor
		* @param image_data_interface	an instance of the image_data interface used to construct this instance
		*/
	    image_data_interface(image_data_interface&&) noexcept = default;
	    image_data_interface(const image_data_interface&) = delete;
		/** Move assignment operator
		* @param image_data_interface	an instance of the image_data interface assigned to this instance
		*/
	    image_data_interface& operator=(image_data_interface&&) noexcept = default;
	    image_data_interface& operator=(const image_data_interface&) = delete;
		
		/** Load image resource into memory
		* perform this as an extra step to facilitate caching of image resources
		*/
		virtual void load() = 0;

		/** Get image format
		* @return the Vulkan image format of the image data
		*/
		virtual vk::Format get_format() const = 0;

		/** Get image type
		* @return the Vulkan image type of the image data
		*/
		virtual vk::ImageType target() const = 0;

		/** Get extent of image data in pixels for the given mipmap level
		* @param aLevel	The mipmap level of the image data, for image data with mipmap levels; must be 0 for image data without mipmap levels.
		* @return the 1D, 2D, or 3D size of the image data, depending on its type
		*/
		virtual extent_type extent(const uint32_t aLevel = 0) const = 0;

		// Note: the return type cannot be const void* because this would result in a compile-time error with the deserializer;
		// The function cannot be const either or gli would call a function overload that returns a const void*
		/** Get pointer to raw image data
		* @param aLayer	The layer of the image data, for layered image data corresponding to texture arrays; must be 0 for image data without layers.
		* @param aFace	The face of the image data, for image data representing cubemaps and cube map arrays; must be 0 for non-cube map image data.
		* @param aLevel	The mipmap level of the image data, for image data with mipmap levels; must be 0 for image data without mipmap levels.
		* @return a pointer to raw image data; the raw data must not be written to
		*/
		virtual void* get_data(const uint32_t aLayer, const uint32_t aFace, const uint32_t aLevel) = 0;

		/** Get size of whole image data, in bytes
		* @return the size of the raw image data array, in bytes
		*/
		virtual size_t size() const = 0;

		/** Get size of one mipmap level of image resource, in bytes
		* @param aLevel	The mipmap level of the image data, for image data with mipmap levels; must be 0 for image data without mipmap levels.
		* @return the size of one mipmap level of the raw image data array, in bytes
		*/
		virtual size_t size(const uint32_t aLevel) const = 0;

		/** Check if image data is empty
		* @return true if the image data is empty, i.e. no data has been loaded
		*/
		virtual bool empty() const = 0;

		/** Get number of mipmap levels in image data
		* @return the number of mipmap levels in the image data; equals 1 if there are no mipmap levels
		*/
		virtual uint32_t levels() const = 0;

		/** Get number of array layers in image data
		* @return the number of array layers in the image data; equals 1 if there if the image data is not an array
		*/
		virtual uint32_t layers() const = 0;

		/** Get number of cube map faces in image data
		* @return the number of cube map faces in the image data; equals 1 if the image data is not a cube map
		*/
		virtual uint32_t faces() const = 0;

		/** Check if image data supports vertical (upside-down) flip on load
		* @return true if the image data supports vertical flipping when loading; this depends on the image format and image library 
		*/
		virtual bool can_flip() const = 0;

		/** Check if image data is in HDR format
		* @return true if the image data is in a HDR format
		*/
		virtual bool is_hdr() const = 0;

		/** Get file path to image data source
		* @return the path to the file which the image data is loaded from
		*/
		std::string path() const
		{
			return mPaths[0];
		}

		/** Get file paths to image data sources of cubemaps
		* @return the paths to the files which the image data of cube maps are loaded from
		*/
		std::vector<std::string> paths() const
		{
			return mPaths;
		}

	protected:
		
		/** Protected constructor, used by abstraction and implementor of the image_data_interface
		* @param aPath					file name of a texture file to load the image data from
		* @param aLoadHdrIfPossible		load the texture as HDR (high dynamic range) data, if supported by the image loading library. If set to true, the image data may be returned in a HDR format even if the texture file does not contain HDR data. If set to false, the image data may be returned in an LDR format even if the texture contains HDR data. It is therefore advised to set this parameter according to the data format of the texture file.
		* @param aLoadSrgbIfApplicable	load the texture as sRGB color-corrected data, if supported by the image loading library. If set to true, the image data may be returned in an sRGB format even if the texture file does not contain sRGB data. If set to false, the image data may be returned in a plain RGB format even if the texture contains sRGB data. It is therefore advised to set this parameter according to the color space of the texture file.
		* @param aFlip					flip the image vertically (upside-down) if set to true. This may be needed if the layout of the image data in the texture file does not match the texture coordinates with which it is used. This parameter may not be supported for all image loaders and texture formats, in particular for some compressed textures.
		* @param aPreferredNumberOfTextureComponents	defines the number of color channels in the returned image_data. The default of 4 corresponds to RGBA texture components. A value of 1 and 2 denote grey value and grey value with alpha, respectively. If the texture file does not contain an alpha channel, the result will be fully opaque. Note that many Vulkan implementations only support textures with RGBA components. This parameter may be ignored by the image loader.
		*/
		image_data_interface(const std::string& aPath, const bool aLoadHdrIfPossible = false, const bool aLoadSrgbIfApplicable = false, const bool aFlip = false, const int aPreferredNumberOfTextureComponents = 4)
			: mPaths({ aPath }), mLoadHdrIfPossible(aLoadHdrIfPossible), mLoadSrgbIfApplicable(aLoadSrgbIfApplicable), mFlip(aFlip), mPreferredNumberOfTextureComponents(aPreferredNumberOfTextureComponents)
		{
		}
		
		/** Protected constructor for cube map image data referencing six individual image files, used by abstraction and implementor of the image data interface
		* @param aPaths					a vector of file names of texture files to load the image data from. The vector must contain six file names, each specifying one side of a cube map texture, in the order +X, -X, +Y, -Y, +Z, -Z. The image data from all files must have the same dimensions and texture formats, after possible HDR and sRGB conversions.
		* @param aLoadHdrIfPossible		load the texture as HDR (high dynamic range) data, if supported by the image loading library. If set to true, the image data may be returned in a HDR format even if the texture file does not contain HDR data. If set to false, the image data may be returned in an LDR format even if the texture contains HDR data. It is therefore advised to set this parameter according to the data format of the texture file.
		* @param aLoadSrgbIfApplicable	load the texture as sRGB color-corrected data, if supported by the image loading library. If set to true, the image data may be returned in an sRGB format even if the texture file does not contain sRGB data. If set to false, the image data may be returned in a plain RGB format even if the texture contains sRGB data. It is therefore advised to set this parameter according to the color space of the texture file.
		* @param aFlip					flip the image vertically (upside-down) if set to true. This may be needed if the layout of the image data in the texture file does not match the texture coordinates with which it is used. This parameter may not be supported for all image loaders and texture formats, in particular for some compressed textures.
		* @param aPreferredNumberOfTextureComponents	defines the number of color channels in the returned image_data. The default of 4 corresponds to RGBA texture components. A value of 1 and 2 denote grey value and grey value with alpha, respectively. If the texture file does not contain an alpha channel, the result will be fully opaque. Note that many Vulkan implementations only support textures with RGBA components. This parameter may be ignored by the image loader.
		*/
		image_data_interface(const std::vector<std::string>& aPaths, const bool aLoadHdrIfPossible = false, const bool aLoadSrgbIfApplicable = false, const bool aFlip = false, const int aPreferredNumberOfTextureComponents = 4)
			: mPaths(aPaths), mLoadHdrIfPossible(aLoadHdrIfPossible), mLoadSrgbIfApplicable(aLoadSrgbIfApplicable), mFlip(aFlip), mPreferredNumberOfTextureComponents(aPreferredNumberOfTextureComponents)
		{
		}

		/** Load cube map image data from six individual image files using one of the available image loading libraries
		* @param aPath					file name of a texture file to load the image data from
		* @param aLoadHdrIfPossible		load the texture as HDR (high dynamic range) data, if supported by the image loading library. If set to true, the image data may be returned in a HDR format even if the texture file does not contain HDR data. If set to false, the image data may be returned in an LDR format even if the texture contains HDR data. It is therefore advised to set this parameter according to the data format of the texture file.
		* @param aLoadSrgbIfApplicable	load the texture as sRGB color-corrected data, if supported by the image loading library. If set to true, the image data may be returned in an sRGB format even if the texture file does not contain sRGB data. If set to false, the image data may be returned in a plain RGB format even if the texture contains sRGB data. It is therefore advised to set this parameter according to the color space of the texture file.
		* @param aFlip					flip the image vertically (upside-down) if set to true. This may be needed if the layout of the image data in the texture file does not match the texture coordinates with which it is used. This parameter may not be supported for all image loaders and texture formats, in particular for some compressed textures.
		* @param aPreferredNumberOfTextureComponents	defines the number of color channels in the returned image_data. The default of 4 corresponds to RGBA texture components. A value of 1 and 2 denote grey value and grey value with alpha, respectively. If the texture file does not contain an alpha channel, the result will be fully opaque. Note that many Vulkan implementations only support textures with RGBA components. This parameter may be ignored by the image loader.
		* @return a pointer to an image data implementor instance that references the given image files
		*/
		static std::unique_ptr<image_data_implementor> load_image_data_from_file(const std::string& aPath, const bool aLoadHdrIfPossible = true, const bool aLoadSrgbIfApplicable = true, const bool aFlip = true, const int aPreferredNumberOfTextureComponents = 4);
		
		/** Load cube map image data from six individual image files using one of the available image loading libraries
		* @param aPaths					a vector of file names of texture files to load the image data from. The vector must contain six file names, each specifying one side of a cube map texture, in the order +X, -X, +Y, -Y, +Z, -Z. The image data from all files must have the same dimensions and texture formats, after possible HDR and sRGB conversions.
		* @param aLoadHdrIfPossible		load the texture as HDR (high dynamic range) data, if supported by the image loading library. If set to true, the image data may be returned in a HDR format even if the texture file does not contain HDR data. If set to false, the image data may be returned in an LDR format even if the texture contains HDR data. It is therefore advised to set this parameter according to the data format of the texture file.
		* @param aLoadSrgbIfApplicable	load the texture as sRGB color-corrected data, if supported by the image loading library. If set to true, the image data may be returned in an sRGB format even if the texture file does not contain sRGB data. If set to false, the image data may be returned in a plain RGB format even if the texture contains sRGB data. It is therefore advised to set this parameter according to the color space of the texture file.
		* @param aFlip					flip the image vertically (upside-down) if set to true. This may be needed if the layout of the image data in the texture file does not match the texture coordinates with which it is used. This parameter may not be supported for all image loaders and texture formats, in particular for some compressed textures.
		* @param aPreferredNumberOfTextureComponents	defines the number of color channels in the returned image_data. The default of 4 corresponds to RGBA texture components. A value of 1 and 2 denote grey value and grey value with alpha, respectively. If the texture file does not contain an alpha channel, the result will be fully opaque. Note that many Vulkan implementations only support textures with RGBA components. This parameter may be ignored by the image loader.
		* @return a pointer to an image data implementor instance that references the given image files
		*/
		static std::unique_ptr<image_data_implementor> load_image_data_from_file(const std::vector<std::string>& aPaths, const bool aLoadHdrIfPossible = true, const bool aLoadSrgbIfApplicable = true, const bool aFlip = true, const int aPreferredNumberOfTextureComponents = 4);

		std::vector<std::string> mPaths;

		bool mLoadHdrIfPossible;
		bool mLoadSrgbIfApplicable;
		// if image should be flipped vertically when loaded, if possible
		bool mFlip;
		int mPreferredNumberOfTextureComponents;
	};

	/** Abstract base class of implementors of image data bridge pattern
	* This class should only be derived by classes implementing support for additional image loading libraries or specialized functionality.
	*/
	class image_data_implementor : public image_data_interface
	{
	public:
		// Default implementations for subclasses that don't support these optional features

		virtual uint32_t levels() const 
		{
			return 1;
		}

		virtual uint32_t layers() const
		{
			return 1;
		}

		virtual uint32_t faces() const
		{
			return 1;
		}

		virtual bool can_flip() const
		{
			return false;
		}

		virtual bool is_hdr() const
		{
			return false;
		}

	protected:
		/** Protected constructor, used by derived classes
		* @param aPath					file name of a texture file to load the image data from
		* @param aLoadHdrIfPossible		load the texture as HDR (high dynamic range) data, if supported by the image loading library. If set to true, the image data may be returned in a HDR format even if the texture file does not contain HDR data. If set to false, the image data may be returned in an LDR format even if the texture contains HDR data. It is therefore advised to set this parameter according to the data format of the texture file.
		* @param aLoadSrgbIfApplicable	load the texture as sRGB color-corrected data, if supported by the image loading library. If set to true, the image data may be returned in an sRGB format even if the texture file does not contain sRGB data. If set to false, the image data may be returned in a plain RGB format even if the texture contains sRGB data. It is therefore advised to set this parameter according to the color space of the texture file.
		* @param aFlip					flip the image vertically (upside-down) if set to true. This may be needed if the layout of the image data in the texture file does not match the texture coordinates with which it is used. This parameter may not be supported for all image loaders and texture formats, in particular for some compressed textures.
		* @param aPreferredNumberOfTextureComponents	defines the number of color channels in the returned image_data. The default of 4 corresponds to RGBA texture components. A value of 1 and 2 denote grey value and grey value with alpha, respectively. If the texture file does not contain an alpha channel, the result will be fully opaque. Note that many Vulkan implementations only support textures with RGBA components. This parameter may be ignored by the image loader.
		*/
		explicit image_data_implementor(const std::string& aPath, const bool aLoadHdrIfPossible = false, const bool aLoadSrgbIfApplicable = false, const bool aFlip = false, const int aPreferredNumberOfTextureComponents = 4)
			: image_data_interface(aPath, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents)
		{
		}

		/** Protected constructor for cube map image data referencing six individual image files, used by derived classes
		* @param aPaths					a vector of file names of texture files to load the image data from. The vector must contain six file names, each specifying one side of a cube map texture, in the order +X, -X, +Y, -Y, +Z, -Z. The image data from all files must have the same dimensions and texture formats, after possible HDR and sRGB conversions.
		* @param aLoadHdrIfPossible		load the texture as HDR (high dynamic range) data, if supported by the image loading library. If set to true, the image data may be returned in a HDR format even if the texture file does not contain HDR data. If set to false, the image data may be returned in an LDR format even if the texture contains HDR data. It is therefore advised to set this parameter according to the data format of the texture file.
		* @param aLoadSrgbIfApplicable	load the texture as sRGB color-corrected data, if supported by the image loading library. If set to true, the image data may be returned in an sRGB format even if the texture file does not contain sRGB data. If set to false, the image data may be returned in a plain RGB format even if the texture contains sRGB data. It is therefore advised to set this parameter according to the color space of the texture file.
		* @param aFlip					flip the image vertically (upside-down) if set to true. This may be needed if the layout of the image data in the texture file does not match the texture coordinates with which it is used. This parameter may not be supported for all image loaders and texture formats, in particular for some compressed textures.
		* @param aPreferredNumberOfTextureComponents	defines the number of color channels in the returned image_data. The default of 4 corresponds to RGBA texture components. A value of 1 and 2 denote grey value and grey value with alpha, respectively. If the texture file does not contain an alpha channel, the result will be fully opaque. Note that many Vulkan implementations only support textures with RGBA components. This parameter may be ignored by the image loader.
		*/
		explicit image_data_implementor(const std::vector<std::string>& aPaths, const bool aLoadHdrIfPossible = false, const bool aLoadSrgbIfApplicable = false, const bool aFlip = false, const int aPreferredNumberOfTextureComponents = 4)
			: image_data_interface(aPaths, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents)
		{
		}
	};

	/** Class representing image data loaded from files
	* This is the image data class intended for direct use in the framework.
	* It can also be used as the base class for specialized abstractions in the image data bridge pattern.
	*/
	class image_data : public image_data_interface
	{
	public:
		/** Public constructor for image data referencing a single image file
		* @param aPath					file name of a texture file to load the image data from
		* @param aLoadHdrIfPossible		load the texture as HDR (high dynamic range) data, if supported by the image loading library. If set to true, the image data may be returned in a HDR format even if the texture file does not contain HDR data. If set to false, the image data may be returned in an LDR format even if the texture contains HDR data. It is therefore advised to set this parameter according to the data format of the texture file.
		* @param aLoadSrgbIfApplicable	load the texture as sRGB color-corrected data, if supported by the image loading library. If set to true, the image data may be returned in an sRGB format even if the texture file does not contain sRGB data. If set to false, the image data may be returned in a plain RGB format even if the texture contains sRGB data. It is therefore advised to set this parameter according to the color space of the texture file.
		* @param aFlip					flip the image vertically (upside-down) if set to true. This may be needed if the layout of the image data in the texture file does not match the texture coordinates with which it is used. This parameter may not be supported for all image loaders and texture formats, in particular for some compressed textures.
		* @param aPreferredNumberOfTextureComponents	defines the number of color channels in the returned image_data. The default of 4 corresponds to RGBA texture components. A value of 1 and 2 denote grey value and grey value with alpha, respectively. If the texture file does not contain an alpha channel, the result will be fully opaque. Note that many Vulkan implementations only support textures with RGBA components. This parameter may be ignored by the image loader.
		*/
		explicit image_data(const std::string& aPath, const bool aLoadHdrIfPossible = false, const bool aLoadSrgbIfApplicable = false, const bool aFlip = false, const int aPreferredNumberOfTextureComponents = 4)
			: image_data_interface(aPath, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents), pimpl(nullptr)
		{
		}

		/** Public constructor for cube map image data referencing six individual image files
		* @param aPaths					a vector of file names of texture files to load the image data from. The vector must contain six file names, each specifying one side of a cube map texture, in the order +X, -X, +Y, -Y, +Z, -Z. The image data from all files must have the same dimensions and texture formats, after possible HDR and sRGB conversions.
		* @param aLoadHdrIfPossible		load the texture as HDR (high dynamic range) data, if supported by the image loading library. If set to true, the image data may be returned in a HDR format even if the texture file does not contain HDR data. If set to false, the image data may be returned in an LDR format even if the texture contains HDR data. It is therefore advised to set this parameter according to the data format of the texture file.
		* @param aLoadSrgbIfApplicable	load the texture as sRGB color-corrected data, if supported by the image loading library. If set to true, the image data may be returned in an sRGB format even if the texture file does not contain sRGB data. If set to false, the image data may be returned in a plain RGB format even if the texture contains sRGB data. It is therefore advised to set this parameter according to the color space of the texture file.
		* @param aFlip					flip the image vertically (upside-down) if set to true. This may be needed if the layout of the image data in the texture file does not match the texture coordinates with which it is used. This parameter may not be supported for all image loaders and texture formats, in particular for some compressed textures.
		* @param aPreferredNumberOfTextureComponents	defines the number of color channels in the returned image_data. The default of 4 corresponds to RGBA texture components. A value of 1 and 2 denote grey value and grey value with alpha, respectively. If the texture file does not contain an alpha channel, the result will be fully opaque. Note that many Vulkan implementations only support textures with RGBA components. This parameter may be ignored by the image loader.
		*/
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

		virtual uint32_t layers() const
		{
			assert(!empty());

			return pimpl->layers();
		}

		virtual uint32_t faces() const
		{
			assert(!empty());

			return pimpl->faces();
		}

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
		// for the pimpl (pointer-to-implementation) idiom, the following should hold true: 
		// use unique_ptr
		// allocate pimpl in out-of-line constructor
		// deallocate in out-of-line destructor (since the complete type is only known after class definition)
		// for user-defined destructor, there is no compiler-generated copy constructor and move-assignment operator; define out-of-line if needed
		std::unique_ptr<image_data_implementor> pimpl;
	};
}
