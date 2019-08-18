#pragma once

namespace cgb
{
	/** Configuration struct to specify the filtering strategy for texture sampling. */
	enum struct filter_mode
	{
		nearest_neighbor,
		bilinear,
		trilinear,
		cubic,
		anisotropic_2x,
		anisotropic_4x,
		anisotropic_8x,
		anisotropic_16x,
	};

	/** Configuration struct to specify the border handling strategy for texture sampling. */
	enum struct border_handling_mode
	{
		clamp_to_edge,
		mirror_clamp_to_edge,
		clamp_to_border,
		repeat,
		mirrored_repeat,
	};

	/** Sampler which is used to configure how to sample from an image. */
	class sampler_t
	{
	public:
		sampler_t() = default;
		sampler_t(const sampler_t&) = delete;
		sampler_t(sampler_t&&) = default;
		sampler_t& operator=(const sampler_t&) = delete;
		sampler_t& operator=(sampler_t&&) = default;
		~sampler_t() = default;

		const auto& config() const { return mInfo; }
		auto& config() { return mInfo; }
		const auto& handle() const { return mSampler.get(); }
		const auto& descriptor_info() const		{ return mDescriptorInfo; }
		const auto& descriptor_type() const		{ return mDescriptorType; }

		/**	Create a new sampler with the given configuration parameters
		 *	@param	pFilterMode					Filtering strategy for the sampler to be created
		 *	@param	pBorderHandlingMode			Border handling strategy for the sampler to be created
		 *	@param	pAlterConfigBeforeCreation	A context-specific function which allows to alter the configuration before the sampler is created.
		 */
		static owning_resource<sampler_t> create(filter_mode pFilterMode, border_handling_mode pBorderHandlingMode, context_specific_function<void(sampler_t&)> pAlterConfigBeforeCreation = {});

	private:
		// Sampler creation configuration
		vk::SamplerCreateInfo mInfo;
		// Sampler handle. It will contain a valid handle only after successful sampler creation.
		vk::UniqueSampler mSampler;
		vk::DescriptorImageInfo mDescriptorInfo;
		vk::DescriptorType mDescriptorType;
		context_tracker<sampler_t> mTracker;
	};

	/** Typedef representing any kind of OWNING sampler representations. */
	using sampler = owning_resource<sampler_t>;

}
