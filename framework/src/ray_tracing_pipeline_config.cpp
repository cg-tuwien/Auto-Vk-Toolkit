#include <cg_base.hpp>

namespace cgb
{
	triangles_hit_group triangles_hit_group::create_with_rahit_only(shader_info _AnyHitShader)
	{
		return triangles_hit_group { std::move(_AnyHitShader), std::nullopt };
	}

	triangles_hit_group triangles_hit_group::create_with_rchit_only(shader_info _ClosestHitShader)
	{
		return triangles_hit_group { std::nullopt, std::move(_ClosestHitShader) };
	}

	triangles_hit_group triangles_hit_group::create_with_rahit_and_rchit(shader_info _AnyHitShader, shader_info _ClosestHitShader)
	{
		return triangles_hit_group { std::move(_AnyHitShader), std::move(_ClosestHitShader) };
	}

	procedural_hit_group procedural_hit_group::create_with_rint_only(shader_info _IntersectionShader)
	{
		return procedural_hit_group { std::move(_IntersectionShader), std::nullopt, std::nullopt };
	}

	procedural_hit_group procedural_hit_group::create_with_rint_and_rahit(shader_info _IntersectionShader, shader_info _AnyHitShader)
	{
		return procedural_hit_group { std::move(_IntersectionShader), std::move(_AnyHitShader), std::nullopt };
	}

	procedural_hit_group procedural_hit_group::create_with_rint_and_rchit(shader_info _IntersectionShader, shader_info _ClosestHitShader)
	{
		return procedural_hit_group { std::move(_IntersectionShader), std::nullopt, std::move(_ClosestHitShader) };
	}

	procedural_hit_group procedural_hit_group::create_with_rint_and_rahit_and_rchit(shader_info _IntersectionShader, shader_info _AnyHitShader, shader_info _ClosestHitShader)
	{
		return procedural_hit_group { std::move(_IntersectionShader), std::move(_AnyHitShader), std::move(_ClosestHitShader) };
	}

	max_recursion_depth max_recursion_depth::disable_recursion()
	{
		return max_recursion_depth { 0u };
	}

	max_recursion_depth max_recursion_depth::set_to(uint32_t _Value)
	{
		return max_recursion_depth { _Value };
	}

	ray_tracing_pipeline_config::ray_tracing_pipeline_config()
		: mPipelineSettings{ cfg::pipeline_settings::nothing }
		, mShaderTableConfig{ }
		, mMaxRecursionDepth{ max_recursion_depth::set_to_max() }
	{

	}
}
