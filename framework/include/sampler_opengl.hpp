#pragma once

namespace cgb
{
	/** Sampler which is used to configure how to sample from an image. */
	class sampler_t
	{
	public:
		sampler_t() = default;
		sampler_t(sampler_t&&) noexcept = default;
		sampler_t(const sampler_t&) = delete;
		sampler_t& operator=(sampler_t&&) noexcept = default;
		sampler_t& operator=(const sampler_t&) = delete;
		~sampler_t() = default;

		static owning_resource<sampler_t> create(filter_mode pFilterMode, border_handling_mode pBorderHandlingMode, context_specific_function<void(sampler_t&)> pAlterConfigBeforeCreation = {});

		context_tracker<sampler_t> mTracker;
	};

	/** Typedef representing any kind of OWNING sampler representations. */
	using sampler = owning_resource<sampler_t>;

}
