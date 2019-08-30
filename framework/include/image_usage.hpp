#pragma once

namespace cgb
{
	enum struct image_usage
	{
		/** The image can be used as the source of a transfer operation. */
		transfer_source							= 0x0001,

		/** The image can be used as the source of a transfer operation. */
		transfer_destination					= 0x0002,

		/** The image can be used to sample from using a sampler. */
		sampled									= 0x0004,

		/** The image can be used for shader load/store operations. */
		shader_storage							= 0x0008,

		/** The image can be used as a color attachment to a framebuffer. */
		color_attachment						= 0x0010,

		/** The image can be used as a depth/stencil attachment to a framebuffer. */
		depth_stencil_attachment				= 0x0020,

		/** The image can be used as an input attachment to a pipeline. */
		input_attachment						= 0x0040,

		/** The image can be used as variable rate shading image. */
		shading_rate_image						= 0x0080,

		/** The image will only be read from. */
		read_only								= 0x0100,

		/** The image can be used for presenting a presentable image for display. */
		presentable								= 0x0200,

		/** valid only for shared presentable images */
		shared_presentable						= 0x0400,

		/** The image shall be internally stored in a format which is optimal for tiling. */
		tiling_optimal							= 0x0800,

		/** The image shall be stored internally in linear tiling format (i.e. row-major + possible some padding) */
		tiling_linear							= 0x1000,

		/** The image will be backed by sparse memory binding. */
		sparse_memory_binding					= 0x2000,

		/** The image can be used to create an image view of a cube map-type */
		cube_compatible							= 0x4000,

		/** In protected memory. */
		is_protected							= 0x8000,

		// v== Some convenience-predefines ==v

		/** The image can be used to transfer from and to, be sampled and serve for shader image/store operations. It is not read-only. */
		versatile_image							= transfer_source | transfer_destination | sampled | shader_storage,

		/** The image can be used as a color attachment, to transfer from and to, be sampled and serve for shader image/store operations. It is not read-only. */
		versatile_color_attachment				= versatile_image | color_attachment,

		/** The image can be used as a depth/stencil attachment, to transfer from and to, be sampled and serve for shader image/store operations. It is not read-only. */
		versatile_depth_stencil_attachment		= versatile_image | depth_stencil_attachment,

		/** The image can be used as read-only depth/stencil attachment, to transfer from, to sample and serve for readonly shader image/store operations. */
		read_only_depth_stencil_attachment		= transfer_source | sampled | shader_storage | read_only | depth_stencil_attachment,

		/** The image can be used as an input attachment, to transfer from and to, be sampled and serve for shader image/store operations. It is not read-only. */
		versatile_input_attachment				= versatile_image | input_attachment,

		/** The image can be used as a shading rate image, to transfer from and to, be sampled and serve for shader image/store operations. It is not read-only. */
		versatile_shading_rate_image			= versatile_image | shading_rate_image,
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
}
