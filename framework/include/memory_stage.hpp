#pragma once

namespace cgb
{
	enum struct memory_stage
	{
        indirect_command				= 0x000001,
		index							= 0x000002,
		vertex_attribute				= 0x000004,
		uniform							= 0x000008,
		input_attachment				= 0x000010,
		color_attachment				= 0x000020,
		color_attachment_noncoherent	= 0x000040,
		depth_stencil_attachment		= 0x000080,
		transfer						= 0x000100,
		conditional_rendering			= 0x000200,
		transform_feedback				= 0x000400,
		command_process					= 0x000800,
		shading_rate_image				= 0x001000,
		acceleration_structure			= 0x002000,
		fragment_density_map			= 0x004000,
		shader_access					= 0x008000,
		host_access						= 0x010000,
		any_access						= 0x020000
	};

	inline memory_stage operator| (memory_stage a, memory_stage b)
	{
		typedef std::underlying_type<memory_stage>::type EnumType;
		return static_cast<memory_stage>(static_cast<EnumType>(a) | static_cast<EnumType>(b));
	}

	inline memory_stage operator& (memory_stage a, memory_stage b)
	{
		typedef std::underlying_type<memory_stage>::type EnumType;
		return static_cast<memory_stage>(static_cast<EnumType>(a) & static_cast<EnumType>(b));
	}

	inline memory_stage& operator |= (memory_stage& a, memory_stage b)
	{
		return a = a | b;
	}

	inline memory_stage& operator &= (memory_stage& a, memory_stage b)
	{
		return a = a & b;
	}

	inline memory_stage exclude(memory_stage original, memory_stage toExclude)
	{
		typedef std::underlying_type<memory_stage>::type EnumType;
		return static_cast<memory_stage>(static_cast<EnumType>(original) & (~static_cast<EnumType>(toExclude)));
	}

	inline bool is_included(const memory_stage toTest, const memory_stage includee)
	{
		return (toTest & includee) == includee;
	}
}
