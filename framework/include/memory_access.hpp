#pragma once

namespace cgb
{
	enum struct memory_access
	{
		indirect_command_data_read_access			= 0x00000001,
		index_buffer_read_access					= 0x00000002,
		vertex_buffer_read_access					= 0x00000004,
		uniform_buffer_read_access					= 0x00000008,
		input_attachment_read_access				= 0x00000010,
		shader_buffers_and_images_read_access		= 0x00000020,
		shader_buffers_and_images_write_access		= 0x00000040,
		color_attachment_read_access				= 0x00000080,
		color_attachment_write_access				= 0x00000100,
		depth_stencil_attachment_read_access		= 0x00000200,
		depth_stencil_attachment_write_access		= 0x00000400,
		transfer_read_access						= 0x00000800,
		transfer_write_access						= 0x00001000,
		host_read_access						    = 0x00002000,
		host_write_access						    = 0x00004000,
		any_read_access								= 0x00008000,
		any_write_access							= 0x00010000,
		transform_feedback_write_access				= 0x00020000,
		transform_feedback_counter_read_access		= 0x00040000,
		transform_feedback_counter_write_access		= 0x00080000,
		conditional_rendering_predicate_read_access = 0x00100000,
		command_process_read_access					= 0x00200000,
		command_process_write_access				= 0x00400000,
		color_attachment_noncoherent_read_access	= 0x00800000,
		shading_rate_image_read_access				= 0x01000000,
		acceleration_structure_read_access			= 0x02000000,
		acceleration_structure_write_access			= 0x04000000,
		fragment_density_map_attachment_read_access = 0x08000000,

		any_vertex_input_read_access				= index_buffer_read_access | vertex_buffer_read_access,
		any_shader_input_read_access				= any_vertex_input_read_access | input_attachment_read_access | uniform_buffer_read_access | shader_buffers_and_images_read_access,
		shader_buffers_and_images_any_access		= shader_buffers_and_images_read_access | shader_buffers_and_images_write_access,
		color_attachment_any_access					= color_attachment_read_access | color_attachment_write_access,
		depth_stencil_attachment_any_access			= depth_stencil_attachment_read_access | depth_stencil_attachment_write_access,
		transfer_any_access							= transfer_read_access | transfer_write_access,
		host_any_access								= host_read_access | host_write_access,
		any_access									= any_read_access | any_write_access,
		any_device_read_access_to_image				= shader_buffers_and_images_read_access | color_attachment_read_access | depth_stencil_attachment_read_access | transfer_read_access | color_attachment_noncoherent_read_access | shading_rate_image_read_access,
		any_device_write_access_to_image			= shader_buffers_and_images_write_access | color_attachment_write_access | depth_stencil_attachment_write_access | transfer_write_access,
		any_device_access_to_image					= shader_buffers_and_images_any_access | color_attachment_any_access | depth_stencil_attachment_any_access | transfer_any_access | color_attachment_noncoherent_read_access | shading_rate_image_read_access,
		command_process_any_access					= command_process_read_access | command_process_write_access,
		acceleration_structure_any_access			= acceleration_structure_read_access | acceleration_structure_write_access
	};

	inline memory_access operator| (memory_access a, memory_access b)
	{
		typedef std::underlying_type<memory_access>::type EnumType;
		return static_cast<memory_access>(static_cast<EnumType>(a) | static_cast<EnumType>(b));
	}

	inline memory_access operator& (memory_access a, memory_access b)
	{
		typedef std::underlying_type<memory_access>::type EnumType;
		return static_cast<memory_access>(static_cast<EnumType>(a) & static_cast<EnumType>(b));
	}

	inline memory_access& operator |= (memory_access& a, memory_access b)
	{
		return a = a | b;
	}

	inline memory_access& operator &= (memory_access& a, memory_access b)
	{
		return a = a & b;
	}

	inline memory_access exclude(memory_access original, memory_access toExclude)
	{
		typedef std::underlying_type<memory_access>::type EnumType;
		return static_cast<memory_access>(static_cast<EnumType>(original) & (~static_cast<EnumType>(toExclude)));
	}

	inline bool is_included(const memory_access toTest, const memory_access includee)
	{
		return (toTest & includee) == includee;
	}
}
