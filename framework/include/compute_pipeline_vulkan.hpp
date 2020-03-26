#pragma once

namespace cgb
{
	/** Represents data for a vulkan compute pipeline */
	class compute_pipeline_t
	{
	public:
		compute_pipeline_t() noexcept = default;
		compute_pipeline_t(compute_pipeline_t&&) noexcept = default;
		compute_pipeline_t(const compute_pipeline_t&) = delete;
		compute_pipeline_t& operator=(compute_pipeline_t&&) noexcept = default;
		compute_pipeline_t& operator=(const compute_pipeline_t&) = delete;
		~compute_pipeline_t() = default;

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
}
