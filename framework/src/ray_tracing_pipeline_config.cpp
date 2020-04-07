#include <cg_base.hpp>

namespace cgb
{
	triangles_hit_group triangles_hit_group::create_with_rahit_only(shader_info _AnyHitShader)
	{
		if (_AnyHitShader.mShaderType != shader_type::any_hit) {
			throw cgb::runtime_error("Shader is not of type shader_type::any_hit");
		}
		return triangles_hit_group { std::move(_AnyHitShader), std::nullopt };
	}

	triangles_hit_group triangles_hit_group::create_with_rchit_only(shader_info _ClosestHitShader)
	{
		if (_ClosestHitShader.mShaderType != shader_type::closest_hit) {
			throw cgb::runtime_error("Shader is not of type shader_type::closest_hit");
		}
		return triangles_hit_group { std::nullopt, std::move(_ClosestHitShader) };
	}

	triangles_hit_group triangles_hit_group::create_with_rahit_and_rchit(shader_info _AnyHitShader, shader_info _ClosestHitShader)
	{
		if (_AnyHitShader.mShaderType != shader_type::any_hit) {
			throw cgb::runtime_error("Shader is not of type shader_type::any_hit");
		}
		if (_ClosestHitShader.mShaderType != shader_type::closest_hit) {
			throw cgb::runtime_error("Shader is not of type shader_type::closest_hit");
		}
		return triangles_hit_group { std::move(_AnyHitShader), std::move(_ClosestHitShader) };
	}

	triangles_hit_group triangles_hit_group::create_with_rahit_only(std::string _AnyHitShaderPath)
	{
		return create_with_rahit_only(
			shader_info::create(std::move(_AnyHitShaderPath), "main", false, shader_type::any_hit)
		);
	}

	triangles_hit_group triangles_hit_group::create_with_rchit_only(std::string _ClosestHitShaderPath)
	{
		return create_with_rchit_only(
			shader_info::create(std::move(_ClosestHitShaderPath), "main", false, shader_type::closest_hit)
		);
	}

	triangles_hit_group triangles_hit_group::create_with_rahit_and_rchit(std::string _AnyHitShaderPath, std::string _ClosestHitShaderPath)
	{
		return create_with_rahit_and_rchit(
			shader_info::create(std::move(_AnyHitShaderPath), "main", false, shader_type::any_hit),
			shader_info::create(std::move(_ClosestHitShaderPath), "main", false, shader_type::closest_hit)
		);
	}


	procedural_hit_group procedural_hit_group::create_with_rint_only(shader_info _IntersectionShader)
	{
		if (_IntersectionShader.mShaderType != shader_type::intersection) {
			throw cgb::runtime_error("Shader is not of type shader_type::intersection");
		}
		return procedural_hit_group { std::move(_IntersectionShader), std::nullopt, std::nullopt };
	}

	procedural_hit_group procedural_hit_group::create_with_rint_and_rahit(shader_info _IntersectionShader, shader_info _AnyHitShader)
	{
		if (_IntersectionShader.mShaderType != shader_type::intersection) {
			throw cgb::runtime_error("Shader is not of type shader_type::intersection");
		}
		if (_AnyHitShader.mShaderType != shader_type::any_hit) {
			throw cgb::runtime_error("Shader is not of type shader_type::any_hit");
		}
		return procedural_hit_group { std::move(_IntersectionShader), std::move(_AnyHitShader), std::nullopt };
	}

	procedural_hit_group procedural_hit_group::create_with_rint_and_rchit(shader_info _IntersectionShader, shader_info _ClosestHitShader)
	{
		if (_IntersectionShader.mShaderType != shader_type::intersection) {
			throw cgb::runtime_error("Shader is not of type shader_type::intersection");
		}
		if (_ClosestHitShader.mShaderType != shader_type::closest_hit) {
			throw cgb::runtime_error("Shader is not of type shader_type::closest_hit");
		}
		return procedural_hit_group { std::move(_IntersectionShader), std::nullopt, std::move(_ClosestHitShader) };
	}

	procedural_hit_group procedural_hit_group::create_with_rint_and_rahit_and_rchit(shader_info _IntersectionShader, shader_info _AnyHitShader, shader_info _ClosestHitShader)
	{
		if (_IntersectionShader.mShaderType != shader_type::intersection) {
			throw cgb::runtime_error("Shader is not of type shader_type::intersection");
		}
		if (_AnyHitShader.mShaderType != shader_type::any_hit) {
			throw cgb::runtime_error("Shader is not of type shader_type::any_hit");
		}
		if (_ClosestHitShader.mShaderType != shader_type::closest_hit) {
			throw cgb::runtime_error("Shader is not of type shader_type::closest_hit");
		}
		return procedural_hit_group { std::move(_IntersectionShader), std::move(_AnyHitShader), std::move(_ClosestHitShader) };
	}

	procedural_hit_group procedural_hit_group::create_with_rint_only(std::string _IntersectionShader)
	{
		return create_with_rint_only(
			shader_info::create(std::move(_IntersectionShader), "main", false, shader_type::intersection)
		);
	}

	procedural_hit_group procedural_hit_group::create_with_rint_and_rahit(std::string _IntersectionShader, std::string _AnyHitShader)
	{
		return create_with_rint_and_rahit(
			shader_info::create(std::move(_IntersectionShader), "main", false, shader_type::intersection),
			shader_info::create(std::move(_AnyHitShader), "main", false, shader_type::any_hit)
		);
	}

	procedural_hit_group procedural_hit_group::create_with_rint_and_rchit(std::string _IntersectionShader, std::string _ClosestHitShader)
	{
		return create_with_rint_and_rchit(
			shader_info::create(std::move(_IntersectionShader), "main", false, shader_type::intersection),
			shader_info::create(std::move(_ClosestHitShader), "main", false, shader_type::closest_hit)
		);
	}

	procedural_hit_group procedural_hit_group::create_with_rint_and_rahit_and_rchit(std::string _IntersectionShader, std::string _AnyHitShader, std::string _ClosestHitShader)
	{
		return create_with_rint_and_rahit_and_rchit(
			shader_info::create(std::move(_IntersectionShader), "main", false, shader_type::intersection),
			shader_info::create(std::move(_AnyHitShader), "main", false, shader_type::any_hit),
			shader_info::create(std::move(_ClosestHitShader), "main", false, shader_type::closest_hit)
		);
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
