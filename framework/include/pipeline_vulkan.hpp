#pragma once

namespace cgb
{
	/** Represents data for a vulkan graphics pipeline 
	*	The data held by such a struct is a triple of:
	*    - render pass
	*    - pipeline layout
	*    - pipeline handle
	*/
	class graphics_pipeline_t
	{
	public:
		graphics_pipeline_t() = default;
		graphics_pipeline_t(graphics_pipeline_t&&) = default;
		graphics_pipeline_t(const graphics_pipeline_t&) = delete;
		graphics_pipeline_t& operator=(graphics_pipeline_t&&) = default;
		graphics_pipeline_t& operator=(const graphics_pipeline_t&) = delete;
		~graphics_pipeline_t() = default;

		const auto& layout_handle() const { return mPipelineLayout.get(); }
		const auto& handle() const { return mPipeline.get(); }
		const auto& renderpass_handle() const { return (*mRenderPass).handle(); }

		static owning_resource<graphics_pipeline_t> create(graphics_pipeline_config _Config, cgb::context_specific_function<void(graphics_pipeline_t&)> _AlterConfigBeforeCreation = {});

	private:
		renderpass mRenderPass;
		uint32_t mSubpassIndex;
		// The vertex input data:
		std::vector<vk::VertexInputBindingDescription> mVertexInputBindingDescriptions;
		std::vector<vk::VertexInputAttributeDescription> mVertexInputAttributeDescriptions;
		vk::PipelineVertexInputStateCreateInfo mPipelineVertexInputStateCreateInfo;
		// How to interpret the vertex input:
		vk::PipelineInputAssemblyStateCreateInfo mInputAssemblyStateCreateInfo;
		// Our precious GPU shader programs:
		std::vector<shader> mShaders;
		std::vector<vk::PipelineShaderStageCreateInfo> mShaderStageCreateInfos;
		// Viewport, depth, and scissors configuration
		std::vector<vk::Viewport> mViewports;
		std::vector<vk::Rect2D> mScissors;
		vk::PipelineViewportStateCreateInfo mViewportStateCreateInfo;
		// Rasterization state:
		vk::PipelineRasterizationStateCreateInfo mRasterizationStateCreateInfo;
		// Depth stencil config:
		vk::PipelineDepthStencilStateCreateInfo mDepthStencilConfig;
		// Color blend attachments
		std::vector<vk::PipelineColorBlendAttachmentState> mBlendingConfigsForColorAttachments;
		vk::PipelineColorBlendStateCreateInfo mColorBlendStateCreateInfo;
		// Multisample state
		vk::PipelineMultisampleStateCreateInfo mMultisampleStateCreateInfo;
		// Dynamic state
		std::vector<vk::DynamicState> mDynamicStateEntries;
		vk::PipelineDynamicStateCreateInfo mDynamicStateCreateInfo;
		// Pipeline layout, i.e. resource bindings
		set_of_descriptor_set_layouts mAllDescriptorSetLayouts;
		std::vector<vk::PushConstantRange> mPushConstantRanges;
		vk::PipelineLayoutCreateInfo mPipelineLayoutCreateInfo;

		// Where all comes together:
		vk::PipelineCreateFlags mPipelineCreateFlags;

		// Handles:
		vk::UniquePipelineLayout mPipelineLayout;
		vk::UniquePipeline mPipeline;
		context_tracker<graphics_pipeline_t> mTracker;
	};

	using graphics_pipeline = owning_resource<graphics_pipeline_t>;
}
