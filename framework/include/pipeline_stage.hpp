#pragma once

namespace cgb
{
	enum struct pipeline_stage
	{
        top_of_pipe							= 0x00000001,
		draw_indirect						= 0x00000002,
		vertex_input						= 0x00000004,
		vertex_shader						= 0x00000008,
		tessellation_control_shader			= 0x00000010,
		tessellation_evaluation_shader		= 0x00000020,
		geometry_shader						= 0x00000040,
		fragment_shader						= 0x00000080,
		early_fragment_tests				= 0x00000100,
		late_fragment_tests					= 0x00000200,
		color_attachment_output				= 0x00000400,
		compute_shader						= 0x00000800,
		transfer							= 0x00001000,
		bottom_of_pipe						= 0x00002000,
		host								= 0x00004000,
		all_graphics_stages					= 0x00008000,
		all_commands						= 0x00010000,
		transform_feedback					= 0x00020000,
		conditional_rendering				= 0x00040000,
		command_preprocess					= 0x00080000,
		shading_rate_image					= 0x00100000,
		ray_tracing_shaders					= 0x00200000,
		acceleration_structure_build		= 0x00400000,
		task_shader							= 0x00800000,
		mesh_shader							= 0x01000000,
		fragment_density_process			= 0x02000000,
	};

	inline pipeline_stage operator| (pipeline_stage a, pipeline_stage b)
	{
		typedef std::underlying_type<pipeline_stage>::type EnumType;
		return static_cast<pipeline_stage>(static_cast<EnumType>(a) | static_cast<EnumType>(b));
	}

	inline pipeline_stage operator& (pipeline_stage a, pipeline_stage b)
	{
		typedef std::underlying_type<pipeline_stage>::type EnumType;
		return static_cast<pipeline_stage>(static_cast<EnumType>(a) & static_cast<EnumType>(b));
	}

	inline pipeline_stage& operator |= (pipeline_stage& a, pipeline_stage b)
	{
		return a = a | b;
	}

	inline pipeline_stage& operator &= (pipeline_stage& a, pipeline_stage b)
	{
		return a = a & b;
	}

	inline pipeline_stage exclude(pipeline_stage original, pipeline_stage toExclude)
	{
		typedef std::underlying_type<pipeline_stage>::type EnumType;
		return static_cast<pipeline_stage>(static_cast<EnumType>(original) & (~static_cast<EnumType>(toExclude)));
	}

	inline bool is_included(const pipeline_stage toTest, const pipeline_stage includee)
	{
		return (toTest & includee) == includee;
	}
}
