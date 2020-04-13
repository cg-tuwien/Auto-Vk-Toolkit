#pragma once

namespace cgb // ========================== TODO/WIP =================================
{
	// Forward declare:
	struct queue_submit_proxy;

	/** Represents a device queue, storing the queue itself, 
	*	the queue family's index, and the queue's index.
	*/
	class device_queue
	{
	public:
		/** Contains all the prepared queues which will be passed to logical device creation. */
		static std::deque<device_queue> sPreparedQueues;

		/** Prepare another queue and eventually add it to `sPreparedQueues`. */
		static device_queue* prepare(
			vk::QueueFlags aFlagsRequired,
			device_queue_selection_strategy aSelectionStrategy,
			std::optional<vk::SurfaceKHR> aSupportForSurface);

		/** Create a new queue on the logical device. */
		static device_queue create(uint32_t aQueueFamilyIndex, uint32_t aQueueIndex);
		/** Create a new queue on the logical device. */
		static device_queue create(const device_queue& aPreparedQueue);

		/** Gets the queue family index of this queue */
		auto family_index() const { return mQueueFamilyIndex; }
		/** Gets queue index (inside the queue family) of this queue. */
		auto queue_index() const { return mQueueIndex; }
		auto priority() const { return mPriority; }
		const auto& handle() const { return mQueue; }
		const auto* handle_addr() const { return &mQueue; }

		/** Gets a pool for the given flags which is usable for this queue and the current thread. */
		command_pool& pool_for(vk::CommandPoolCreateFlags aFlags) const;

		/** Creates a "standard" command buffer which is not necessarily short-lived
		 *	and can be re-submitted, but not necessarily re-recorded.
		 *
		 *	@param	aSimultaneousUseEnabled		`true` means that the command buffer to be created can be 
		 *										resubmitted to a queue while it is in the pending state.
		 *										It also means that it can be recorded into multiple primary
		 *										command buffers, if it is intended to be used as a secondary.
		 */
		command_buffer create_command_buffer(bool aSimultaneousUseEnabled = false) const;

		/** Creates a "standard" command buffer which is not necessarily short-lived
		 *	and can be re-submitted, but not necessarily re-recorded.
		 *
		 *	@param	aNumBuffers					How many command buffers to be created.
		 *	@param	aSimultaneousUseEnabled		`true` means that the command buffer to be created can be 
		 *										resubmitted to a queue while it is in the pending state.
		 *										It also means that it can be recorded into multiple primary
		 *										command buffers, if it is intended to be used as a secondary.
		 */
		std::vector<command_buffer> create_command_buffers(uint32_t aNumBuffers, bool aSimultaneousUseEnabled = false) const;
		
		/** Creates a command buffer which is intended to be used as a one time submit command buffer */
		command_buffer create_single_use_command_buffer() const;
		
		/** Creates a command buffer which is intended to be used as a one time submit command buffer
		 *	@param	aNumBuffers					How many command buffers to be created.
		 */
		std::vector<command_buffer> create_single_use_command_buffers(uint32_t aNumBuffers) const;

		/** Creates a command buffer which is intended to be reset (and possible re-recorded). */
		command_buffer create_resettable_command_buffer(bool aSimultaneousUseEnabled = false) const;
		
		/** Creates a command buffer which is intended to be reset (and possible re-recorded).
		 *	@param	aNumBuffers					How many command buffers to be created.
		 */
		std::vector<command_buffer> create_resettable_command_buffers(uint32_t aNumBuffers, bool aSimultaneousUseEnabled = false) const;

		/** TODO */
		void submit(command_buffer_t& aCommandBuffer);
		
		/** TODO */
		void submit(std::vector<std::reference_wrapper<command_buffer_t>> aCommandBuffers);

		/** TODO */
		fence submit_with_fence(command_buffer_t& aCommandBuffer, std::vector<semaphore> aWaitSemaphores = {});
		
		/** TODO */
		fence submit_with_fence(std::vector<std::reference_wrapper<command_buffer_t>> aCommandBuffers, std::vector<semaphore> aWaitSemaphores = {});

		/** TODO */
		semaphore submit_and_handle_with_semaphore(command_buffer aCommandBuffer, std::vector<semaphore> aWaitSemaphores = {});
		semaphore submit_and_handle_with_semaphore(std::optional<command_buffer> aCommandBuffer, std::vector<semaphore> aWaitSemaphores = {});
		
		/** TODO */
		semaphore submit_and_handle_with_semaphore(std::vector<command_buffer> aCommandBuffers, std::vector<semaphore> aWaitSemaphores = {});
		
	private:
		uint32_t mQueueFamilyIndex;
		uint32_t mQueueIndex;
		float mPriority;
		vk::Queue mQueue;
	};

	static bool operator==(const device_queue& left, const device_queue& right)
	{
		const auto same = left.family_index() == right.family_index() && left.queue_index() == right.queue_index();
		assert(!same || left.priority() == right.priority());
		assert(!same || left.handle() == right.handle());
		return same;
	}

	struct queue_submit_proxy
	{
		queue_submit_proxy() = default;
		queue_submit_proxy(queue_submit_proxy&&) = delete;
		queue_submit_proxy(const queue_submit_proxy&) = delete;
		queue_submit_proxy& operator=(queue_submit_proxy&&) = delete;
		queue_submit_proxy& operator=(const queue_submit_proxy&) = delete;

		device_queue& mQueue;
		vk::SubmitInfo mSubmitInfo;
		std::vector<command_buffer> mCommandBuffers;
		std::vector<semaphore> mWaitSemaphores;
		std::vector<semaphore> mSignalSemaphores;
	};
}
