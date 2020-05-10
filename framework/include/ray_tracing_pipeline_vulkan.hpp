#pragma once

namespace cgb
{
	/** Represents data for a vulkan ray tracing pipeline */
	class ray_tracing_pipeline_t
	{
	public:
		ray_tracing_pipeline_t() = default;
		ray_tracing_pipeline_t(ray_tracing_pipeline_t&&) noexcept = default;
		ray_tracing_pipeline_t(const ray_tracing_pipeline_t&) = delete;
		ray_tracing_pipeline_t& operator=(ray_tracing_pipeline_t&&) noexcept = default;
		ray_tracing_pipeline_t& operator=(const ray_tracing_pipeline_t&) = delete;
		~ray_tracing_pipeline_t() = default;

		const auto& layout_handle() const { return mPipelineLayout.get(); }
		std::tuple<const ray_tracing_pipeline_t*, const vk::PipelineLayout, const std::vector<vk::PushConstantRange>*> layout() const { return std::make_tuple(this, layout_handle(), &mPushConstantRanges); }
		const auto& handle() const { return mPipeline; }
		vk::DeviceSize table_entry_size() const { return static_cast<vk::DeviceSize>(mShaderGroupHandleSize); }
		vk::DeviceSize table_size() const { return static_cast<vk::DeviceSize>(mShaderBindingTable->meta_data().total_size()); }
		const auto& shader_binding_table_handle() const { return mShaderBindingTable->buffer_handle(); }

		static owning_resource<ray_tracing_pipeline_t> create(ray_tracing_pipeline_config _Config, cgb::context_specific_function<void(ray_tracing_pipeline_t&)> _AlterConfigBeforeCreation = {});

	private:
		// TODO: What to do with flags?
		vk::PipelineCreateFlags mPipelineCreateFlags;

		// Our precious GPU shader programs:
		std::vector<shader> mShaders;
		std::vector<vk::PipelineShaderStageCreateInfo> mShaderStageCreateInfos;

		// Shader table a.k.a. shader groups:
		std::vector<vk::RayTracingShaderGroupCreateInfoKHR> mShaderGroupCreateInfos;

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
		//vk::ResultValueType<vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderDynamic>>::type mPipeline;
		vk::Pipeline mPipeline;

		size_t mShaderGroupHandleSize;
		cgb::generic_buffer mShaderBindingTable; // TODO: support more than one shader binding table

		context_tracker<ray_tracing_pipeline_t> mTracker;
	};

	using ray_tracing_pipeline = owning_resource<ray_tracing_pipeline_t>;
	
	template <>
	inline void command_buffer_t::bind_pipeline<ray_tracing_pipeline_t>(const ray_tracing_pipeline_t& aPipeline)
	{
		handle().bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, aPipeline.handle());
	}

	template <>
	inline void command_buffer_t::bind_pipeline<ray_tracing_pipeline>(const ray_tracing_pipeline& aPipeline)
	{
		bind_pipeline<ray_tracing_pipeline_t>(aPipeline);
	}

	template <>
	inline void command_buffer_t::bind_descriptors<std::tuple<const ray_tracing_pipeline_t*, const vk::PipelineLayout, const std::vector<vk::PushConstantRange>*>>
		(std::tuple<const ray_tracing_pipeline_t*, const vk::PipelineLayout, const std::vector<vk::PushConstantRange>*> aPipelineLayout, std::initializer_list<binding_data> aBindings, descriptor_cache_interface* aDescriptorCache)
	{
		command_buffer_t::bind_descriptors(vk::PipelineBindPoint::eRayTracingKHR, std::get<const ray_tracing_pipeline_t*>(aPipelineLayout)->layout_handle(), std::move(aBindings), aDescriptorCache);
	}

}
