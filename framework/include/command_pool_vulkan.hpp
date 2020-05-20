#pragma once

namespace cgb
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
		command_pool(command_pool&&) noexcept = default;
		command_pool& operator=(const command_pool&) = delete;
		command_pool& operator=(command_pool&&) noexcept = default;
		~command_pool() = default;

		auto queue_family_index() const { return mQueueFamilyIndex; }
		const auto& create_info() const { return mCreateInfo; }
		const auto& handle() const { return mCommandPool.get(); }
		const auto* handle_addr() const { return &mCommandPool.get(); }

		static command_pool create(uint32_t aQueueFamilyIndex, vk::CommandPoolCreateFlags aCreateFlags = vk::CommandPoolCreateFlags());

		std::vector<command_buffer> get_command_buffers(uint32_t aCount, vk::CommandBufferUsageFlags aUsageFlags = vk::CommandBufferUsageFlags(), bool aPrimary = true);
		command_buffer get_command_buffer(vk::CommandBufferUsageFlags aUsageFlags = vk::CommandBufferUsageFlags(), bool aPrimary = true);

		/** Creates a "standard" command buffer which is not necessarily short-lived
		 *	and can be re-submitted, but not necessarily re-recorded.
		 *
		 *	@param	aTargetQueue				The queue, the created command buffer shall be submitted to.
		 *	@param	aSimultaneousUseEnabled		`true` means that the command buffer to be created can be 
		 *										resubmitted to a queue while it is in the pending state.
		 *										It also means that it can be recorded into multiple primary
		 *										command buffers, if it is intended to be used as a secondary.
		 */
		static command_buffer create_command_buffer(cgb::device_queue& aTargetQueue, bool aSimultaneousUseEnabled = false, bool aPrimary = true);

		/** Creates a "standard" command buffer which is not necessarily short-lived
		 *	and can be re-submitted, but not necessarily re-recorded.
		 *
		 *	@param	aTargetQueue				The queue, the created command buffer shall be submitted to.
		 *	@param	aNumBuffers					How many command buffers to be created.
		 *	@param	aSimultaneousUseEnabled		`true` means that the command buffer to be created can be 
		 *										resubmitted to a queue while it is in the pending state.
		 *										It also means that it can be recorded into multiple primary
		 *										command buffers, if it is intended to be used as a secondary.
		 */
		static std::vector<command_buffer> create_command_buffers(cgb::device_queue& aTargetQueue, uint32_t aNumBuffers, bool aSimultaneousUseEnabled = false, bool aPrimary = true);

		/** Creates a command buffer which is intended to be used as a one time submit command buffer
		 *	@param	aTargetQueue				The queue, the created command buffer shall be submitted to.
		 */
		static command_buffer create_single_use_command_buffer(cgb::device_queue& aTargetQueue, bool aPrimary = true);

		/** Creates a command buffer which is intended to be used as a one time submit command buffer
		 *	@param	aTargetQueue				The queue, the created command buffer shall be submitted to.
		 *	@param	aNumBuffers					How many command buffers to be created.
		 */
		static std::vector<command_buffer> create_single_use_command_buffers(cgb::device_queue& aTargetQueue, uint32_t aNumBuffers, bool aPrimary = true);

		/** Creates a command buffer which is intended to be reset (and possible re-recorded).
		 *	@param	aTargetQueue				The queue, the created command buffer shall be submitted to.
		 *	@param	aSimultaneousUseEnabled		`true` means that the command buffer to be created can be 
		 *										resubmitted to a queue while it is in the pending state.
		 *										It also means that it can be recorded into multiple primary
		 *										command buffers, if it is intended to be used as a secondary.
		 */
		static command_buffer create_resettable_command_buffer(cgb::device_queue& aTargetQueue, bool aSimultaneousUseEnabled = false, bool aPrimary = true);

		/** Creates a command buffer which is intended to be reset (and possible re-recorded).
		 *	@param	aTargetQueue				The queue, the created command buffer shall be submitted to.
		 *	@param	aNumBuffers					How many command buffers to be created.
		 *	@param	aSimultaneousUseEnabled		`true` means that the command buffer to be created can be 
		 *										resubmitted to a queue while it is in the pending state.
		 *										It also means that it can be recorded into multiple primary
		 *										command buffers, if it is intended to be used as a secondary.
		 */
		static std::vector<command_buffer> create_resettable_command_buffers(cgb::device_queue& aTargetQueue, uint32_t aNumBuffers, bool aSimultaneousUseEnabled = false, bool aPrimary = true);


	private:
		uint32_t mQueueFamilyIndex;
		vk::CommandPoolCreateInfo mCreateInfo;
		vk::UniqueCommandPool mCommandPool;
	};

}
