namespace cgb
{
	command_pool command_pool::create(uint32_t pQueueFamilyIndex, vk::CommandPoolCreateFlags pCreateFlags)
	{
		auto createInfo = vk::CommandPoolCreateInfo()
			.setQueueFamilyIndex(pQueueFamilyIndex)
			.setFlags(pCreateFlags); // Optional
									 // Possible values for the flags [7]
									 //  - VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: Hint that command buffers are rerecorded with new commands very often (may change memory allocation behavior)
									 //  - VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: Allow command buffers to be rerecorded individually, without this flag they all have to be reset together
		command_pool result;
		result.mQueueFamilyIndex = pQueueFamilyIndex;
		result.mCreateInfo = createInfo;
		result.mCommandPool = context().logical_device().createCommandPoolUnique(createInfo);
		return result;
	}

	std::vector<command_buffer> command_pool::get_command_buffers(uint32_t pCount, vk::CommandBufferUsageFlags pUsageFlags)
	{
		return command_buffer::create_many(pCount, *this, pUsageFlags);
	}

	command_buffer command_pool::get_command_buffer(vk::CommandBufferUsageFlags pUsageFlags)
	{
		return command_buffer::create(*this, pUsageFlags);
	}
}
