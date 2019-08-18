#pragma once

namespace cgb // ========================== TODO/WIP =================================
{

	/** Represents a Vulkan command pool, holds the native handle and takes
	*	care about lifetime management of the native handles.
	*	Also contains the queue family index it has been created for and is 
	*  intended to be used with:
	*	"All command buffers allocated from this command pool must be submitted 
	*	 on queues from the same queue family." [+]
	*	
	*	[+]: https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkCommandPoolCreateInfo.html
	*/
	class command_pool
	{
	public:
		command_pool() = default;
		command_pool(const command_pool&) = delete;
		command_pool(command_pool&&) = default;
		command_pool& operator=(const command_pool&) = delete;
		command_pool& operator=(command_pool&&) = default;
		~command_pool() = default;

		auto queue_family_index() const { return mQueueFamilyIndex; }
		const auto& create_info() const { return mCreateInfo; }
		const auto& handle() const { return mCommandPool.get(); }
		const auto* handle_addr() const { return &mCommandPool.get(); }

		static command_pool create(uint32_t pQueueFamilyIndex, vk::CommandPoolCreateFlags pCreateFlags = vk::CommandPoolCreateFlags());

		std::vector<command_buffer> get_command_buffers(uint32_t pCount, vk::CommandBufferUsageFlags pUsageFlags = vk::CommandBufferUsageFlags());
		command_buffer get_command_buffer(vk::CommandBufferUsageFlags pUsageFlags = vk::CommandBufferUsageFlags());

	private:
		uint32_t mQueueFamilyIndex;
		vk::CommandPoolCreateInfo mCreateInfo;
		vk::UniqueCommandPool mCommandPool;
	};

}
