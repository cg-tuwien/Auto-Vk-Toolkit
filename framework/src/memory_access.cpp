#include <cg_base.hpp>

namespace cgb
{
	bool is_read_access(memory_access aValue)
	{
		switch (aValue) {
			case memory_access::indirect_command_data_read_access			:
			case memory_access::index_buffer_read_access					:
			case memory_access::vertex_buffer_read_access					:
			case memory_access::uniform_buffer_read_access					:
			case memory_access::input_attachment_read_access				:
			case memory_access::shader_buffers_and_images_read_access		:
			case memory_access::color_attachment_read_access				:
			case memory_access::depth_stencil_attachment_read_access		:
			case memory_access::transfer_read_access						:
			case memory_access::host_read_access						    :
			case memory_access::any_read_access								:
			case memory_access::transform_feedback_counter_read_access		:
			case memory_access::conditional_rendering_predicate_read_access :
			case memory_access::command_process_read_access					:
			case memory_access::color_attachment_noncoherent_read_access	:
			case memory_access::shading_rate_image_read_access				:
			case memory_access::acceleration_structure_read_access			:
			case memory_access::fragment_density_map_attachment_read_access :
			case memory_access::any_vertex_input_read_access				:
			case memory_access::any_shader_input_read_access				:
			case memory_access::any_device_read_access_to_image				:
				return true;
			default:
				return false;
		}	
	}

	read_memory_access::operator memory_access() const
	{
		validate_or_throw();
		return mMemoryAccess;
	}
	
	void read_memory_access::validate_or_throw() const
	{
		if (!is_read_access(mMemoryAccess)) {
			throw std::runtime_error("The access flag represented by this instance of read_memory_access is not a read-type access flag.");
		}
	}

	write_memory_access::operator memory_access() const
	{
		validate_or_throw();
		return mMemoryAccess;
	}
	
	void write_memory_access::validate_or_throw() const
	{
		if (is_read_access(mMemoryAccess)) {
			throw std::runtime_error("The access flag represented by this instance of write_memory_access is not a write-type access flag.");
		}
	}

}
