#pragma once

namespace cgb
{
	class image_sampler_t
	{
	public:
		image_sampler_t() = default;
		image_sampler_t(const image_sampler_t&) = delete;
		image_sampler_t(image_sampler_t&&) = default;
		image_sampler_t& operator=(const image_sampler_t&) = delete;
		image_sampler_t& operator=(image_sampler_t&&) = default;
		~image_sampler_t() = default;

		static owning_resource<image_sampler_t> create(image_view pImageView, sampler pSampler);
	};

	/** Typedef representing any kind of OWNING image-sampler representations. */
	using image_sampler = owning_resource<image_sampler_t>;
}
