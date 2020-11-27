#pragma once
#include <gvk.hpp>

namespace gvk
{
	class image_resource_t;

	typedef std::unique_ptr<image_resource_t> image_resource;


	class image_resource_t
	{
	public:
		typedef vk::Extent3D extent_type;

		virtual ~image_resource_t()
		{
		};

		// FIXME: define consistent meaning of aLoadHdrIfPossible; stb_image converts image data to or from float format, but other image loading libraries, like GLI, do not
		// Likewise, sRGB is ignored for GLI loader
		static image_resource create_image_resource_from_file(const std::string& aPath, bool aLoadHdrIfPossible = true, bool aLoadSrgbIfApplicable = true, bool aFlip = true,
			int aPreferredNumberOfTextureComponents = 4);

		// Order of faces +X, -X, +Y, -Y, +Z, -Z
		static image_resource create_image_resource_from_file(const std::vector<std::string>& aPath, bool aLoadHdrIfPossible = true, bool aLoadSrgbIfApplicable = true, bool aFlip = true,
			int aPreferredNumberOfTextureComponents = 4);

		virtual vk::Format get_format() = 0;

		virtual vk::ImageType target() = 0; 

		virtual extent_type extent() = 0;
		virtual extent_type extent(size_t level) = 0;

		virtual void* get_data(size_t layer, size_t face, size_t level) = 0;
		virtual size_t size(size_t level) = 0;

		virtual bool empty() = 0;

		const std::string path()
		{
			return mPath;
		}

		// Default implementations for subclasses that don't support these optional features

		// Mipmap levels; 1 if no Mipmapping, 0 if Mipmaps should be created after loading
		virtual size_t levels()
		{
			return 1;
		} 

		// array layers, for texture arrays
		virtual size_t layers()
		{
			return 1;
		}

		// faces in cubemap, must be 6 for cubemaps, 1 for anything else
		virtual size_t faces()
		{
			return 1;
		}

		// TODO: make protected or remove?
		virtual bool can_flip()
		{
			return false;
		}

		virtual bool is_hdr()
		{
			return false;
		}

	protected:
		image_resource_t(const std::string& aPath, bool aHDR = false, bool asRGB = false, bool aFlip = false, int aPreferredNumberOfTextureComponents = 4)
			: mPath(aPath), mHDR(aHDR), msRGB(asRGB), mFlip(aFlip), mPreferredNumberOfTextureComponents(aPreferredNumberOfTextureComponents)
		{
		}

		virtual void flip()
		{
			throw std::exception();
		}

		std::string mPath;
		bool mFlip;
		bool mHDR;
		bool msRGB;
		int mPreferredNumberOfTextureComponents;
	};

	class composite_cubemap_image_resource_t : public image_resource_t
	{
	public:
		composite_cubemap_image_resource_t(const std::vector<std::string>& aPaths, bool aHDR = false, bool asRGB = false, bool aFlip = false, int aPreferredNumberOfTextureComponents = 4)
			// TODO: how to handle names of composite?
			: image_resource_t(aPaths[0], aHDR, asRGB, aFlip, aPreferredNumberOfTextureComponents)
		{
			assert(aPaths.size() == 6);

			for (auto path : aPaths)
			{
				image_resource i = create_image_resource_from_file(path, aHDR, asRGB, aFlip, aPreferredNumberOfTextureComponents);

				assert(!i->empty());
				assert(i->layers() == 1);
				// target must be 2D
				assert(i->target() == vk::ImageType::e2D);
				// must not be a cubemap
				assert(i->faces() == 1);

				if (image_resources.size() > 0)
				{
					// all image resources must have the same format, target and extent
					image_resource& i0 = image_resources[0];
					assert(i->get_format() == i0->get_format());
					assert(i->target() == i0->target());
					assert(i->extent() == i0->extent());
					// Mipmap levels must agree
					assert(i->levels() == i0->levels());
				}

				image_resources.push_back(std::move(i));
			}

			assert(image_resources.size() == 6);
		};

		virtual ~composite_cubemap_image_resource_t()
		{
		};

		virtual vk::Format get_format()
		{
			return image_resources[0]->get_format();
		}

		virtual vk::ImageType target()
		{
			return image_resources[0]->target();
		}

		virtual extent_type extent()
		{
			return image_resources[0]->extent();
		}

		virtual extent_type extent(size_t level)
		{
			return image_resources[0]->extent(level);
		}

		virtual void* get_data(size_t layer, size_t face, size_t level)
		{
			//return map_to_resource(layer, face)->get_data(0, 0, level);
			return image_resources[face]->get_data(0, 0, level);
		}

		virtual size_t size(size_t level)
		{
			return image_resources[0]->size(level);
		}

		virtual size_t levels()
		{
			return image_resources[0]->levels();
		}

		virtual size_t layers()
		{
			// TODO support arrays
			return 1;
		}

		// faces in cubemap
		virtual size_t faces()
		{
			return 6;
		}

		virtual bool empty()
		{
			return image_resources[0]->empty();
		}

		virtual bool is_hdr()
		{
			return image_resources[0]->is_hdr();
		}

		const std::string path()
		{
			return mPath;
		}

	private:
		/*
		class image_resource_selection : public image_resource
		{
		public:
			// NOTE selecting individual Mipmap levels or ranges of faces is probably not very useful and thus not supported, aNumFaces should be limited to either 1 or 6?
			image_resource_selection(image_resource& aImageResource, size_t aBaseLayer, size_t aNumLayers = 1, size_t aBaseFace = 0, size_t aNumFaces = 1)
				: mImageResource(aImageResource), mBaseLayer(aBaseLayer), mNumLayers(aNumLayers), mBaseFace(aBaseFace), mNumFaces(aNumFaces)
			{
				assert(mNumLayers > 0);
				assert(mNumFaces == 1 || mNumFaces == 6);
				assert(mBaseLayer + mNumLayers <= mImageResource->layers());
				assert(mBaseFace + mNumFaces <= mImageResource->faces());
			}

			virtual ~image_resource_selection()
			{
			};

			virtual vk::Format get_format()
			{
				return mImageResource->get_format();
			}

			virtual gli::target target()
			{
				return mImageResource->target();
			}

			virtual extent_type extent()
			{
				return mImageResource->extent();
			}

			virtual extent_type extent(size_t level)
			{
				return mImageResource->extent(level);
			}


			virtual void* get_data(size_t layer, size_t face, size_t level)
			{
				return mImageResource->get_data(mBaseLayer + layer, mBaseFace + face, level);
			}
			virtual size_t size(size_t level)
			{
				return mImageResource->size(level);
			}

			virtual size_t levels()
			{
				return mImageResource->levels();
			}
			// array layers, for texture arrays
			virtual size_t layers()
			{
				return mNumLayers;
			}
			// faces in cubemap
			virtual size_t faces()
			{
				return mNumFaces;
			}

			virtual bool empty()
			{
				return mImageResource->empty();
			}

			virtual bool is_hdr()
			{
				return mImageResource->is_hdr();
			}

			const std::string path()
			{
				return mImageResource->path();
			}
		private:
			image_resource& mImageResource;
			size_t mBaseLayer, mNumLayers;
			size_t mBaseFace, mNumFaces;
		};
		*/
		image_resource& map_to_resource(size_t layer, size_t face)
		{
			// TODO: handle cubemap arrays
			assert(layer == 0);

			assert(0 <= face && face < 6);

			return image_resources[face];
		}

		std::vector<image_resource> image_resources;
	};
}