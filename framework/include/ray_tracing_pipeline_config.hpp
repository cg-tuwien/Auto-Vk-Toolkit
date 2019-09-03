#pragma once

namespace cgb
{
	/** Contains shader infos about a Hit Group which uses the
	 *	ray tracing's standard triangles intersection functionality.
	 */
	struct triangles_hit_group
	{
		std::optional<shader_info> mClosestHitShader;
		std::optional<shader_info> mAnyHitShader;
	};

	/** Contains shader infos about a Hit Group which uses a custom,
	 *	"procedural" intersection shader. Hence, the `mIntersectionShader` 
	 *	member must be set while the other members are optional.
	 */
	struct procedural_hit_group
	{
		std::optional<shader_info> mClosestHitShader;
		std::optional<shader_info> mAnyHitShader;
		shader_info mIntersectionShader;
	};

	/**	Represents one entry of the "shader table" which is used with
	 *	ray tracing pipelines. The "shader table" is also commonly known
	 *	as "shader groups". 
	 *	Every "shader table entry" represents either a single shader,
	 *	or one of the two types of "hit groups", namely `triangles_hit_group`,
	 *	or `procedural_hit_group`.
	 */
	using shader_table_entry_config = std::variant<shader_info, triangles_hit_group, procedural_hit_group>;
	
	/** Represents the entire "shader table" config, which consists
	 *	of multiple "shader table entries", each one represented by
	 *	an element of type `cgb::shader_table_entry_config`.
	 *	Depending on specific usages of the shader table, the order 
	 *	of the shader table entries matters and hence, is retained 
	 *	like configured in the vector of shader table entries: `mShaderTableEntries`.
	 */
	struct shader_table_config
	{
		std::vector<shader_table_entry_config> mShaderTableEntries;
	};

	/** Pipeline configuration data: COMPUTE PIPELINE CONFIG STRUCT */
	struct ray_tracing_pipeline_config
	{
		ray_tracing_pipeline_config();
		ray_tracing_pipeline_config(ray_tracing_pipeline_config&&) = default;
		ray_tracing_pipeline_config(const ray_tracing_pipeline_config&) = delete;
		ray_tracing_pipeline_config& operator=(ray_tracing_pipeline_config&&) = default;
		ray_tracing_pipeline_config& operator=(const ray_tracing_pipeline_config&) = delete;
		~ray_tracing_pipeline_config() = default;

		cfg::pipeline_settings mPipelineSettings; // ?
		shader_table_config mShaderTableConfig;
		uint32_t mMaxRecursionDepth;
		std::vector<binding_data> mResourceBindings;
		std::vector<push_constant_binding_data> mPushConstantsBindings;
	};
}
