#pragma once

namespace cgb
{
	class device_queue;
	
	class sync
	{
	public:
		sync() = default;
		sync(const sync&) = delete;
		sync(sync&&) noexcept = default;
		sync& operator=(const sync&) = delete;
		sync& operator=(sync&&) noexcept = default;

		static sync wait_idle();
		static sync with_semaphore(std::function<void(owning_resource<semaphore_t>)> aSemaphoreHandler, std::vector<semaphore> aWaitSemaphores = {});
		static sync with_semaphore_on_current_frame(std::vector<semaphore> aWaitSemaphores = {}, cgb::window* aWindow = nullptr);
		static sync with_barrier(/* TODO: Proceed here with barrier-parameters */, std::function<void(command_buffer)> aCommandBufferLifetimeHandler);
		static sync with_barrier_on_current_frame(std::function<void(command_buffer&)> aEstablishBarrierCallback, cgb::window* aWindow = nullptr);

		sync& on_queue(std::reference_wrapper<device_queue> aQueue);
		sync& then_transfer_to(std::reference_wrapper<device_queue> aQueue);

		std::reference_wrapper<device_queue> queue_to_use() const;
		std::reference_wrapper<device_queue> queue_to_transfer_to() const;
		bool queues_are_the_same() const;

		void handle_before_end_of_recording(command_buffer& aCommandBuffer) const;
		void handle_after_end_of_recording(command_buffer aCommandBuffer);
		
	private:
		std::function<void(owning_resource<semaphore_t>)> mSemaphoreHandler;
		std::vector<semaphore> mWaitSemaphores;
		std::function<void(command_buffer&)> mEstablishBarrierCallback;
		std::function<void(command_buffer)> mCommandBufferLifetimeHandler;
		std::optional<std::reference_wrapper<device_queue>> mQueueToUse;
		std::optional<std::reference_wrapper<device_queue>> mQueueToTransferOwnershipTo;
		std::optional<vk::PipelineStageFlags> mSrcStage; // TODO: Not sure if we can really use this reasonably in cgb::sync
		std::optional<vk::PipelineStageFlags> mDstStage; // TODO: Not sure if we can really use this reasonably in cgb::sync
		std::optional<vk::AccessFlags> mSrcAccess; // TODO: Not sure if we can really use this reasonably in cgb::sync
		std::optional<vk::AccessFlags> mDstAccess; // TODO: Not sure if we can really use this reasonably in cgb::sync
	};
}
