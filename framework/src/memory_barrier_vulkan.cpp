#include <cg_base.hpp>

namespace cgb
{
	memory_barrier memory_barrier::create_read_after_write()
	{
		memory_barrier result;
		result.mBarrier = vk::MemoryBarrier{
			get_write_flag_for_memory_stage(memory_stage::any_access),
			get_read_flag_for_memory_stage(memory_stage::any_access)
		};
		return result;
	}
	
	memory_barrier memory_barrier::create_write_after_write()
	{
		memory_barrier result;
		result.mBarrier = vk::MemoryBarrier{
			get_write_flag_for_memory_stage(memory_stage::any_access),
			get_write_flag_for_memory_stage(memory_stage::any_access)
		};
		return result;
	}
	
	memory_barrier memory_barrier::create_read_after_write(memory_stage aMemoryToMakeVisible, memory_stage aMemoryToMakeAvailable)
	{
		memory_barrier result;
		result.mBarrier = vk::MemoryBarrier{
			get_write_flag_for_memory_stage(aMemoryToMakeVisible),
			get_read_flag_for_memory_stage(aMemoryToMakeAvailable)
		};
		return result;
	}
	
	memory_barrier memory_barrier::create_write_after_write(memory_stage aMemoryToMakeVisible, memory_stage aMemoryToMakeAvailable)
	{
		memory_barrier result;
		result.mBarrier = vk::MemoryBarrier{
			get_write_flag_for_memory_stage(aMemoryToMakeVisible),
			get_write_flag_for_memory_stage(aMemoryToMakeAvailable)
		};
		return result;
	}
	
	memory_barrier memory_barrier::create_read_after_write(const image_t& aImage)
	{
		vk::ImageMemoryBarrier{
			get_write_flag_for_memory_stage(memory_stage::any_access),
			get_read_flag_for_memory_stage(memory_stage::any_access),
			aImage.current_layout(),
			aImage.target_layout(),
		};
	}
	
	memory_barrier memory_barrier::create_write_after_write(const image_t& aImage)
	{
		
	}
	
	memory_barrier memory_barrier::create_read_after_write(memory_stage aMemoryToMakeVisible, memory_stage aMemoryToMakeAvailable, const image_t& aImage)
	{
		
	}
	
	memory_barrier memory_barrier::create_write_after_write(memory_stage aMemoryToMakeVisible, memory_stage aMemoryToMakeAvailable, const image_t& aImage)
	{
		
	}

	vk::AccessFlags get_write_flag_for_memory_stage(memory_stage aMemoryStage)
	{
		switch (aMemoryStage) {
		case memory_stage::indirect_command:
			assert(false);
			throw std::logic_error("Invalid write stage.");
		case memory_stage::index:
			assert(false);
			throw std::logic_error("Invalid write stage.");
		case memory_stage::vertex_attribute:
			assert(false);
			throw std::logic_error("Invalid write stage.");
		case memory_stage::uniform:
			assert(false);
			throw std::logic_error("Invalid write stage.");
		case memory_stage::input_attachment:
			return vk::AccessFlagBits::eColorAttachmentWrite;
		case memory_stage::color_attachment:
			return vk::AccessFlagBits::eColorAttachmentWrite;
		case memory_stage::depth_stencil_attachment:
			return vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		case memory_stage::transfer:
			return vk::AccessFlagBits::eTransferWrite;
		case memory_stage::conditional_rendering:
			assert(false);
			throw std::logic_error("Invalid write stage.");
		case memory_stage::transform_feedback:
			return vk::AccessFlagBits::eTransformFeedbackWriteEXT | vk::AccessFlagBits::eTransformFeedbackCounterWriteEXT;
		case memory_stage::command_process:
			return vk::AccessFlagBits::eCommandProcessWriteNVX;
		case memory_stage::shading_rate_image:
			assert(false);
			throw std::logic_error("Invalid write stage.");
		case memory_stage::acceleration_structure:
			return vk::AccessFlagBits::eAccelerationStructureWriteNV;
		case memory_stage::shader_access:
			return vk::AccessFlagBits::eShaderWrite;
		case memory_stage::host_access:
			return vk::AccessFlagBits::eHostWrite;
		case memory_stage::any_access:
			return vk::AccessFlagBits::eMemoryWrite;
		default:
			assert(false);
			throw std::logic_error("Invalid write stage.");
		}
	}

	vk::AccessFlags get_read_flag_for_memory_stage(memory_stage aMemoryStage)
	{
		switch (aMemoryStage) {
		case memory_stage::indirect_command:
			return vk::AccessFlagBits::eIndirectCommandRead;
		case memory_stage::index:
			return vk::AccessFlagBits::eIndexRead;
		case memory_stage::vertex_attribute:
			return vk::AccessFlagBits::eVertexAttributeRead;
		case memory_stage::uniform:
			return vk::AccessFlagBits::eUniformRead;
		case memory_stage::input_attachment:
			return vk::AccessFlagBits::eInputAttachmentRead;
		case memory_stage::color_attachment:
			return vk::AccessFlagBits::eColorAttachmentRead;
		case memory_stage::depth_stencil_attachment:
			return vk::AccessFlagBits::eDepthStencilAttachmentRead;
		case memory_stage::transfer:
			return vk::AccessFlagBits::eTransferRead;
		case memory_stage::conditional_rendering:
			return vk::AccessFlagBits::eConditionalRenderingReadEXT;
		case memory_stage::transform_feedback:
			return vk::AccessFlagBits::eTransformFeedbackCounterReadEXT;
		case memory_stage::command_process:
			return vk::AccessFlagBits::eCommandProcessReadNVX;
		case memory_stage::shading_rate_image:
			return vk::AccessFlagBits::eShadingRateImageReadNV;
		case memory_stage::acceleration_structure:
			return vk::AccessFlagBits::eAccelerationStructureReadNV;
		case memory_stage::shader_access:
			return vk::AccessFlagBits::eShaderRead;
		case memory_stage::host_access:
			return vk::AccessFlagBits::eHostRead;
		case memory_stage::any_access:
			return vk::AccessFlagBits::eMemoryRead;
		default:
			assert(false);
			throw std::logic_error("Invalid read stage.");
		}
	}

}
