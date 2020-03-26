#pragma once

namespace cgb
{
	/** Class representing an image view. */
	class image_view_t
	{
	public:
		image_view_t() = default;
		image_view_t(image_view_t&&) noexcept = default;
		image_view_t(const image_view_t&) = delete;
		image_view_t& operator=(image_view_t&&) noexcept = default;
		image_view_t& operator=(const image_view_t&) = delete;
		~image_view_t() = default;

		static owning_resource<image_view_t> create(cgb::image aImageToOwn, std::optional<image_format> aViewFormat = std::nullopt, context_specific_function<void(image_view_t&)> aAlterConfigBeforeCreation = {});

	};

	/** Typedef representing any kind of OWNING image view representations. */
	using image_view = owning_resource<image_view_t>;
}
