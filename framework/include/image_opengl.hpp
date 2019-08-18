#pragma once

namespace cgb
{
	/** TBD */
	struct image_format
	{
		image_format() noexcept;
		image_format(GLenum pFormat) noexcept;

		GLenum mFormat;
	};

	/** TBD
	 */
	class image_t
	{
	public:
		image_t() = default;
		image_t(const image_t&) = delete;
		image_t(image_t&&) = default;
		image_t& operator=(const image_t&) = delete;
		image_t& operator=(image_t&&) = default;
		~image_t() = default;

		static owning_resource<image_t> create(int pWidth, int pHeight, image_format pFormat, memory_usage pMemoryUsage, bool pUseMipMaps = false, int pNumLayers = 1, context_specific_function<void(image_t&)> pAlterConfigBeforeCreation = {});

		static owning_resource<image_t> create_depth(int pWidth, int pHeight, std::optional<image_format> pFormat = std::nullopt, memory_usage pMemoryUsage = memory_usage::device, bool pUseMipMaps = false, int pNumLayers = 1, context_specific_function<void(image_t&)> pAlterConfigBeforeCreation = {});

		static owning_resource<image_t> create_depth_stencil(int pWidth, int pHeight, std::optional<image_format> pFormat = std::nullopt, memory_usage pMemoryUsage = memory_usage::device, bool pUseMipMaps = false, int pNumLayers = 1, context_specific_function<void(image_t&)> pAlterConfigBeforeCreation = {});

	};

	/** Typedef representing any kind of OWNING image representations. */
	using image	= owning_resource<image_t>;

}
