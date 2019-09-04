#pragma once

namespace cgb
{
	/** Represents data for a vulkan ray tracing pipeline */
	class ray_tracing_pipeline_t
	{
	public:
		ray_tracing_pipeline_t() = default;
		ray_tracing_pipeline_t(ray_tracing_pipeline_t&&) = default;
		ray_tracing_pipeline_t(const ray_tracing_pipeline_t&) = delete;
		ray_tracing_pipeline_t& operator=(ray_tracing_pipeline_t&&) = default;
		ray_tracing_pipeline_t& operator=(const ray_tracing_pipeline_t&) = delete;
		~ray_tracing_pipeline_t() = default;

		const auto& layout_handle() const { return mPipelineLayout.get(); }
		const auto& handle() const { return mPipeline.get(); }

		static owning_resource<ray_tracing_pipeline_t> create(ray_tracing_pipeline_config _Config, cgb::context_specific_function<void(ray_tracing_pipeline_t&)> _AlterConfigBeforeCreation = {});

	private:
		// TODO: What to do with flags?
		vk::PipelineCreateFlags mPipelineCreateFlags;

		// Our precious GPU shader programs:
		std::vector<shader> mShaders;
		std::vector<vk::PipelineShaderStageCreateInfo> mShaderStageCreateInfos;

		// Shader table a.k.a. shader groups:
		std::vector<vk::RayTracingShaderGroupCreateInfoNV> mShaderGroupCreateInfos;

		// Maximum recursion depth:
		uint32_t mMaxRecursionDepth;

		// TODO: What to do with the base pipeline index?
		int32_t mBasePipelineIndex;

		// Pipeline layout, i.e. resource bindings
		set_of_descriptor_set_layouts mAllDescriptorSetLayouts;
		std::vector<vk::PushConstantRange> mPushConstantRanges;
		vk::PipelineLayoutCreateInfo mPipelineLayoutCreateInfo;

		// Handles:
		vk::UniquePipelineLayout mPipelineLayout;
		vk::ResultValueType<vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderDynamic>>::type mPipeline;

		context_tracker<ray_tracing_pipeline_t> mTracker;
	};

	using ray_tracing_pipeline = owning_resource<ray_tracing_pipeline_t>;

}
