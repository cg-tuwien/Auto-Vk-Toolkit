#pragma once

namespace cgb
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
		static void create(device_queue& aPreparedQueue);

		/** Gets the queue family index of this queue */
		auto family_index() const { return mQueueFamilyIndex; }
		/** Gets queue index (inside the queue family) of this queue. */
		auto queue_index() const { return mQueueIndex; }
		auto priority() const { return mPriority; }
		const auto& handle() const { return mQueue; }
		const auto* handle_ptr() const { return &mQueue; }

		/** TODO */
		semaphore submit_with_semaphore(command_buffer_t& aCommandBuffer);
		
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
