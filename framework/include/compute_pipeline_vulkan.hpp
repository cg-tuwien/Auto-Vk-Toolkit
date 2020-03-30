#pragma once

namespace cgb
{
	/** Represents data for a vulkan compute pipeline */
	class compute_pipeline_t
	{
	public:
		compute_pipeline_t() = default;
		compute_pipeline_t(compute_pipeline_t&&) noexcept = default;
		compute_pipeline_t(const compute_pipeline_t&) = delete;
		compute_pipeline_t& operator=(compute_pipeline_t&&) noexcept = default;
		compute_pipeline_t& operator=(const compute_pipeline_t&) = delete;
		~compute_pipeline_t() = default;

		std::tuple<const compute_pipeline_t*, const set_of_descriptor_set_layouts*> layout() const { return std::make_tuple(this, &mAllDescriptorSetLayouts); }
		const auto& layout_handle() const { return mPipelineLayout.get(); }
		const auto& handle() const { return mPipeline.get(); }

		static owning_resource<compute_pipeline_t> create(compute_pipeline_config _Config, cgb::context_specific_function<void(compute_pipeline_t&)> _AlterConfigBeforeCreation = {});

	private:
		// TODO: What to do with flags?
		vk::PipelineCreateFlags mPipelineCreateFlags;

		// Only one shader for compute pipelines:
		shader mShader;
		vk::PipelineShaderStageCreateInfo mShaderStageCreateInfo;

		// TODO: What to do with the base pipeline index?
		int32_t mBasePipelineIndex;

		// Pipeline layout, i.e. resource bindings
		set_of_descriptor_set_layouts mAllDescriptorSetLayouts;
		std::vector<vk::PushConstantRange> mPushConstantRanges;
		vk::PipelineLayoutCreateInfo mPipelineLayoutCreateInfo;

		// Handles:
		vk::UniquePipelineLayout mPipelineLayout;
		vk::UniquePipeline mPipeline;

		context_tracker<compute_pipeline_t> mTracker;
	};
	
	using compute_pipeline = owning_resource<compute_pipeline_t>;

	template <>
	inline void command_buffer_t::bind_pipeline<compute_pipeline_t>(const compute_pipeline_t& aPipeline)
	{
		handle().bindPipeline(vk::PipelineBindPoint::eCompute, aPipeline.handle());
	}

	template <>
	inline void command_buffer_t::bind_pipeline<compute_pipeline>(const compute_pipeline& aPipeline)
	{
		bind_pipeline<compute_pipeline_t>(aPipeline);
	}

	template <>
	inline void command_buffer_t::bind_descriptors<std::tuple<const compute_pipeline_t*, const set_of_descriptor_set_layouts*>>(std::tuple<const compute_pipeline_t*, const set_of_descriptor_set_layouts*> aPipelineLayout, std::initializer_list<binding_data> aBindings, descriptor_cache_interface* aDescriptorCache)
	{
		bind_descriptors(vk::PipelineBindPoint::eCompute, std::get<const compute_pipeline_t*>(aPipelineLayout)->layout_handle(), std::move(aBindings), aDescriptorCache);
	}
}
