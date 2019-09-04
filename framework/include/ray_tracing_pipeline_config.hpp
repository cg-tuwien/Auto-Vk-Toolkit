#pragma once

namespace cgb
{
	/** Contains shader infos about a Hit Group which uses the
	 *	ray tracing's standard triangles intersection functionality.
	 */
	struct triangles_hit_group
	{
		static triangles_hit_group create_with_rahit_only(shader_info _AnyHitShader);
		static triangles_hit_group create_with_rchit_only(shader_info _ClosestHitShader);
		static triangles_hit_group create_with_rahit_and_rchit(shader_info _AnyHitShader, shader_info _ClosestHitShader);

		std::optional<shader_info> mAnyHitShader;
		std::optional<shader_info> mClosestHitShader;
	};

	/** Contains shader infos about a Hit Group which uses a custom,
	 *	"procedural" intersection shader. Hence, the `mIntersectionShader` 
	 *	member must be set while the other members are optional.
	 */
	struct procedural_hit_group
	{
		static procedural_hit_group create_with_rint_only(shader_info _IntersectionShader);
		static procedural_hit_group create_with_rint_and_rahit(shader_info _IntersectionShader, shader_info _AnyHitShader);
		static procedural_hit_group create_with_rint_and_rchit(shader_info _IntersectionShader, shader_info _ClosestHitShader);
		static procedural_hit_group create_with_rint_and_rahit_and_rchit(shader_info _IntersectionShader, shader_info _AnyHitShader, shader_info _ClosestHitShader);

		shader_info mIntersectionShader;
		std::optional<shader_info> mAnyHitShader;
		std::optional<shader_info> mClosestHitShader;
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

	/** Represents the maximum recursion depth supported by a ray tracing pipeline. */
	struct max_recursion_depth
	{
		/** Disable recursions, i.e. set to zero. */
		static max_recursion_depth disable_recursion();
		/** Set the maximum recursion depth to a specific value. */
		static max_recursion_depth set_to(uint32_t _Value);
		/** Set the recursion depth to the maximum which is supported by the physical device. */
		static max_recursion_depth set_to_max();

		uint32_t mMaxRecursionDepth;
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
		max_recursion_depth mMaxRecursionDepth;
		std::vector<binding_data> mResourceBindings;
		std::vector<push_constant_binding_data> mPushConstantsBindings;
	};
}
