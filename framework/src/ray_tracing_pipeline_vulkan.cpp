#include <cg_base.hpp>

namespace cgb
{
	owning_resource<ray_tracing_pipeline_t> ray_tracing_pipeline_t::create(ray_tracing_pipeline_config _Config, cgb::context_specific_function<void(ray_tracing_pipeline_t&)> _AlterConfigBeforeCreation)
	{
		ray_tracing_pipeline_t result;

		// 1. Pipeline Flags
		result.mPipelineCreateFlags;

		// 2. Shaders
		result.mShaders;
		result.mShaderStageCreateInfos;

		// 3. Shader Table
		result.mShaderGroups;
		result.mShaderGroupCreateInfos;

		// 4. Maximum recursion depth:
		result.mMaxRecursionDepth;

		// 5. Pipeline layout
		result.mAllDescriptorSetLayouts;
		result.mPushConstantRanges;
		result.mPipelineLayoutCreateInfo;

		// 6. Maybe alter the config?

		// 7. Build the pipeline
		result.mPipelineLayout;
		result.mPipeline;

		result.mTracker.setTrackee(result);
		return result;
	}
}
