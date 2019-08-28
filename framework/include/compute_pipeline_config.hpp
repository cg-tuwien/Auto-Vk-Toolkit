#pragma once

namespace cgb
{
	/** Pipeline configuration data: COMPUTE PIPELINE CONFIG STRUCT */
	struct compute_pipeline_config
	{
		compute_pipeline_config();
		compute_pipeline_config(compute_pipeline_config&&) = default;
		compute_pipeline_config(const compute_pipeline_config&) = delete;
		compute_pipeline_config& operator=(compute_pipeline_config&&) = default;
		compute_pipeline_config& operator=(const compute_pipeline_config&) = delete;
		~compute_pipeline_config() = default;

		cfg::pipeline_settings mPipelineSettings; // ?
		std::optional<shader_info> mShaderInfo;
		std::vector<binding_data> mResourceBindings;
		std::vector<push_constant_binding_data> mPushConstantsBindings;
	};
}
