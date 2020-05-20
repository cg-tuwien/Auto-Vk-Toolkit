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

	std::vector<command_buffer> command_pool::get_command_buffers(uint32_t aCount, vk::CommandBufferUsageFlags aUsageFlags, bool aPrimary)
	{
		return command_buffer_t::create_many(aCount, *this, aUsageFlags, aPrimary ? vk::CommandBufferLevel::ePrimary : vk::CommandBufferLevel::eSecondary);
	}

	command_buffer command_pool::get_command_buffer(vk::CommandBufferUsageFlags aUsageFlags, bool aPrimary)
	{
		return command_buffer_t::create(*this, aUsageFlags, aPrimary ? vk::CommandBufferLevel::ePrimary : vk::CommandBufferLevel::eSecondary);
	}

	command_buffer command_pool::create_command_buffer(cgb::device_queue& aTargetQueue, bool aSimultaneousUseEnabled, bool aPrimary)
	{
		auto flags = vk::CommandBufferUsageFlags();
		if (aSimultaneousUseEnabled) {
			flags |= vk::CommandBufferUsageFlagBits::eSimultaneousUse;
		}
		auto result = context().get_command_pool_for_queue(aTargetQueue, vk::CommandPoolCreateFlags{}) // no special flags
			.get_command_buffer(flags, aPrimary);
		return result;
	}

	std::vector<command_buffer> command_pool::create_command_buffers(cgb::device_queue& aTargetQueue, uint32_t aNumBuffers, bool aSimultaneousUseEnabled, bool aPrimary)
	{
		auto flags = vk::CommandBufferUsageFlags();
		if (aSimultaneousUseEnabled) {
			flags |= vk::CommandBufferUsageFlagBits::eSimultaneousUse;
		}
		auto result = context().get_command_pool_for_queue(aTargetQueue, vk::CommandPoolCreateFlags{}) // no special flags
			.get_command_buffers(aNumBuffers, flags, aPrimary);
		return result;
	}
	
	command_buffer command_pool::create_single_use_command_buffer(cgb::device_queue& aTargetQueue, bool aPrimary)
	{
		const vk::CommandBufferUsageFlags flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
		auto result = context().get_command_pool_for_queue(aTargetQueue, vk::CommandPoolCreateFlagBits::eTransient)
			.get_command_buffer(flags, aPrimary);
		return result;
	}

	std::vector<command_buffer> command_pool::create_single_use_command_buffers(cgb::device_queue& aTargetQueue, uint32_t aNumBuffers, bool aPrimary)
	{
		const vk::CommandBufferUsageFlags flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
		auto result = context().get_command_pool_for_queue(aTargetQueue, vk::CommandPoolCreateFlagBits::eTransient)
			.get_command_buffers(aNumBuffers, flags, aPrimary);
		return result;		
	}

	command_buffer command_pool::create_resettable_command_buffer(cgb::device_queue& aTargetQueue, bool aSimultaneousUseEnabled, bool aPrimary)
	{
		vk::CommandBufferUsageFlags flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit | vk::CommandBufferUsageFlagBits::eSimultaneousUse;
		if (aSimultaneousUseEnabled) {
			flags |= vk::CommandBufferUsageFlagBits::eSimultaneousUse;
		}
		auto result = context().get_command_pool_for_queue(aTargetQueue, vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
			.get_command_buffer(flags, aPrimary);
		return result;
	}

	std::vector<command_buffer> command_pool::create_resettable_command_buffers(cgb::device_queue& aTargetQueue, uint32_t aNumBuffers, bool aSimultaneousUseEnabled, bool aPrimary)
	{
		vk::CommandBufferUsageFlags flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit | vk::CommandBufferUsageFlagBits::eSimultaneousUse;
		if (aSimultaneousUseEnabled) {
			flags |= vk::CommandBufferUsageFlagBits::eSimultaneousUse;
		}
		auto result = context().get_command_pool_for_queue(aTargetQueue, vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
			.get_command_buffers(aNumBuffers, flags, aPrimary);
		return result;
	}

}
