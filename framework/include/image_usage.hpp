#pragma once

namespace cgb
{
	enum struct image_usage
	{
		/** The image can be used as the source of a transfer operation. */
		transfer_source							= 0x000001,

		/** The image can be used as the source of a transfer operation. */
		transfer_destination					= 0x000002,

		/** The image can be used to sample from using a sampler. */
		sampled									= 0x000004,

		/** The image can be used for shader load/store operations. */
		shader_storage							= 0x000008,

		/** The image can be used as a color attachment to a framebuffer. */
		color_attachment						= 0x000010,

		/** The image can be used as a depth/stencil attachment to a framebuffer. */
		depth_stencil_attachment				= 0x000020,

		/** The image can be used as an input attachment to a pipeline. */
		input_attachment						= 0x000040,

		/** The image can be used as variable rate shading image. */
		shading_rate_image						= 0x000080,

		/** The image will only be read from. */
		read_only								= 0x000100,

		/** The image can be used for presenting a presentable image for display. */
		presentable								= 0x000200,

		/** valid only for shared presentable images */
		shared_presentable						= 0x000400,

		/** The image shall be internally stored in a format which is optimal for tiling. */
		tiling_optimal							= 0x000800,

		/** The image shall be stored internally in linear tiling format (i.e. row-major + possible some padding) */
		tiling_linear							= 0x001000,

		/** The image will be backed by sparse memory binding. */
		sparse_memory_binding					= 0x002000,

		/** The image can be used to create an image view of a cube map-type */
		cube_compatible							= 0x004000,

		/** In protected memory. */
		is_protected							= 0x008000,

		/** Used as a texture with mip mapping enabled */
		mip_mapped								= 0x010000,
		
		// v== Some convenience-predefines ==v

		/** General image usage that can be used for transfer and be sampled, stored in an efficient `tiling_optimal` format. */
		general_image							= transfer_source | transfer_destination | sampled | tiling_optimal,

		/** An image that shall only be used in readonly-mode. */
		read_only_image							= transfer_source | sampled | read_only | tiling_optimal,

		/** An image with the same properties as `general_image` and in addition, can be used for shader storage operations */
		general_storage_image					= general_image | shader_storage, 

		/** An image with the same properties as `general_image` with the `read_only` flag set in addition. */
		general_read_only_image					= general_image | read_only,

		/** A `general_image` intended to be used as a color attachment. */
		general_color_attachment				= general_image | color_attachment,

		/** A color attachment used in read only manner. */
		read_only_color_attachment				= read_only_image | color_attachment,

		/** A `general_image` intended to be used as a depth/stencil attachment. */
		general_depth_stencil_attachment		= general_image | depth_stencil_attachment,

		/** A depth/stencil attachment used in read only manner. */
		read_only_depth_stencil_attachment		= read_only_image | depth_stencil_attachment,

		/** A `general_image` intended to be used as input attachment. */
		general_input_attachment				= general_image | input_attachment,

		/** An input attachment used in read only manner. */
		read_only_input_attachment				= read_only_image | input_attachment,

		/** A `general_image` intended to be used as shading rate image */
		general_shading_rate_image				= general_image | shading_rate_image,

		/** A `general_image` intended to be used as texture and has MIP mapping enabled by default. */
		general_texture							= general_image | mip_mapped,

		/** A `general_texture` intended to be used as cube map. */
		general_cube_map_texture				= general_texture | cube_compatible,
	};

	inline image_usage operator| (image_usage a, image_usage b)
	{
		typedef std::underlying_type<image_usage>::type EnumType;
		return static_cast<image_usage>(static_cast<EnumType>(a) | static_cast<EnumType>(b));
	}

	inline image_usage operator& (image_usage a, image_usage b)
	{
		typedef std::underlying_type<image_usage>::type EnumType;
		return static_cast<image_usage>(static_cast<EnumType>(a) & static_cast<EnumType>(b));
	}

	inline image_usage& operator |= (image_usage& a, image_usage b)
	{
		return a = a | b;
	}

	inline image_usage& operator &= (image_usage& a, image_usage b)
	{
		return a = a & b;
	}

	inline image_usage exclude(image_usage original, image_usage toExclude)
	{
		typedef std::underlying_type<image_usage>::type EnumType;
		return static_cast<image_usage>(static_cast<EnumType>(original) & (~static_cast<EnumType>(toExclude)));
	}
}
