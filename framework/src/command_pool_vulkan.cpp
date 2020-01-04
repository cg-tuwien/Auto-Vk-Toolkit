#include <cg_base.hpp>

namespace cgb
{
	command_pool command_pool::create(uint32_t aQueueFamilyIndex, vk::CommandPoolCreateFlags aCreateFlags)
	{
		auto createInfo = vk::CommandPoolCreateInfo()
			.setQueueFamilyIndex(aQueueFamilyIndex)
			.setFlags(aCreateFlags);
		command_pool result;
		result.mQueueFamilyIndex = aQueueFamilyIndex;
		result.mCreateInfo = createInfo;
		result.mCommandPool = context().logical_device().createCommandPoolUnique(createInfo);
		return result;
	}

	std::vector<command_buffer> command_pool::get_command_buffers(uint32_t aCount, vk::CommandBufferUsageFlags aUsageFlags)
	{
		return command_buffer_t::create_many(aCount, *this, aUsageFlags);
	}

	command_buffer command_pool::get_command_buffer(vk::CommandBufferUsageFlags aUsageFlags)
	{
		return command_buffer_t::create(*this, aUsageFlags);
	}
}
