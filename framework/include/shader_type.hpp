#pragma once

namespace cgb
{
	/** Represents one specific type of shader */
	enum struct shader_type
	{
		/** Vertex Shader */
		vertex					= 0x0001,
		/** Tessellation Control Shader */
		tessellation_control	= 0x0002,
		/** Tessellation Evaluation Shader */
		tessellation_evaluation	= 0x0004,
		/** Geometry Shader */
		geometry				= 0x0008,
		/** Fragment Shader */
		fragment				= 0x0010,
		/** Compute Shader */
		compute					= 0x0020,
		/** Ray Generation Shader (Nvidia RTX) */
		ray_generation			= 0x0040,
		/** Ray Tracing Any Hit Shader  (Nvidia RTX) */
		any_hit					= 0x0080,
		/** Ray Tracing Closest Hit Shader (Nvidia RTX) */
		closest_hit				= 0x0100,
		/** Ray Tracing Miss Shader (Nvidia RTX) */
		miss					= 0x0200,
		/** Ray Tracing Intersection Shader (Nvidia RTX) */
		intersection			= 0x0400,
		/** Ray Tracing Callable Shader (Nvidia RTX) */
		callable				= 0x0800,
		/** Task Shader (Nvidia Turing)  */
		task					= 0x1000,
		/** Mesh Shader (Nvidia Turing)  */
		mesh					= 0x2000,
		/** all of them */
		all						= vertex | tessellation_control | tessellation_evaluation | geometry | fragment | compute | ray_generation | any_hit | closest_hit | miss | intersection | callable | task | mesh,
	};

	inline shader_type operator| (shader_type a, shader_type b)
	{
		typedef std::underlying_type<shader_type>::type EnumType;
		return static_cast<shader_type>(static_cast<EnumType>(a) | static_cast<EnumType>(b));
	}

	inline shader_type operator& (shader_type a, shader_type b)
	{
		typedef std::underlying_type<shader_type>::type EnumType;
		return static_cast<shader_type>(static_cast<EnumType>(a) & static_cast<EnumType>(b));
	}

	inline shader_type& operator |= (shader_type& a, shader_type b)
	{
		return a = a | b;
	}

	inline shader_type& operator &= (shader_type& a, shader_type b)
	{
		return a = a & b;
	}
}
