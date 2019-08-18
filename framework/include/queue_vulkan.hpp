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
			vk::QueueFlags pFlagsRequired,
			device_queue_selection_strategy pSelectionStrategy,
			std::optional<vk::SurfaceKHR> pSupportForSurface);

		/** Create a new queue on the logical device. */
		static device_queue create(uint32_t pQueueFamilyIndex, uint32_t pQueueIndex);
		/** Create a new queue on the logical device. */
		static device_queue create(const device_queue& pPreparedQueue);

		/** Gets the queue family index of this queue */
		auto family_index() const { return mQueueFamilyIndex; }
		/** Gets queue index (inside the queue family) of this queue. */
		auto queue_index() const { return mQueueIndex; }
		auto priority() const { return mPriority; }
		const auto& handle() const { return mQueue; }
		const auto* handle_addr() const { return &mQueue; }

		/** Gets a pool which is usable for this queue and the current thread. */
		command_pool& pool() const;

	private:
		uint32_t mQueueFamilyIndex;
		uint32_t mQueueIndex;
		float mPriority;
		vk::Queue mQueue;
	};

	struct queue_submit_proxy
	{
		queue_submit_proxy() = default;
		queue_submit_proxy(const queue_submit_proxy&) = delete;
		queue_submit_proxy(queue_submit_proxy&&) = delete;
		queue_submit_proxy& operator=(const queue_submit_proxy&) = delete;
		queue_submit_proxy& operator=(queue_submit_proxy&&) = delete;

		device_queue& mQueue;
		vk::SubmitInfo mSubmitInfo;
		std::vector<command_buffer> mCommandBuffers;
		std::vector<semaphore> mWaitSemaphores;
		std::vector<semaphore> mSignalSemaphores;
	};
}
