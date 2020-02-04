#pragma once

namespace cgb
{
	// Forward-declare the device-queue
	class device_queue;

	/** A type which holds all the data passed to a memory barrier.
	 *	Used to alter the config before issuing the actual pipeline barrier.
	 */
	struct pipeline_barrier_parameters
	{
		vk::PipelineStageFlags mSrcStageMask;
		vk::PipelineStageFlags mDstStageMask;
		vk::DependencyFlags mDependencyFlags;
		vk::ArrayProxy<const vk::MemoryBarrier> mMemoryBarriers;
		vk::ArrayProxy<const vk::BufferMemoryBarrier> mBufferMemoryBarriers;
		vk::ArrayProxy<const vk::ImageMemoryBarrier> mImageMemoryBarriers;
	};

	class sync
	{
	public:
		enum struct sync_type { via_wait_idle, via_semaphore, via_barrier };
		
		sync() = default;
		sync(const sync&) = delete;
		sync(sync&&) noexcept = default;
		sync& operator=(const sync&) = delete;
		sync& operator=(sync&&) noexcept = default;

		static sync wait_idle();
		static sync with_semaphore(std::function<void(owning_resource<semaphore_t>)> aSemaphoreHandler, std::vector<semaphore> aWaitSemaphores = {});
		static sync with_semaphore_on_current_frame(std::vector<semaphore> aWaitSemaphores = {}, cgb::window* aWindow = nullptr);
		static sync with_barrier(std::function<void(command_buffer)> aCommandBufferLifetimeHandler);
		static sync with_barrier_on_current_frame(cgb::window* aWindow = nullptr);

		/**	Establish an EXECUTION BARRIER: The specified stage has to wait on the completion of the command to be synchronized.
		 *  The command will set some source stage, this function can be used to set the destination stage of the pipeline execution barrier.
		 *
		 *	This flag applies to:
		 *		- semaphores
		 *		- memory barriers
		 */
		sync& continue_execution_in_stage(vk::PipelineStageFlags aStageWhichHasToWait);

		sync& make_memory_available_for_writing(memory_stage aMemoryToBeMadeAvailable);

		sync& make_memory_available_for_reading(memory_stage aMemoryToBeMadeAvailable);

		sync& make_memory_available_for_writing(memory_stage aMemoryToBeMadeAvailable, const image_t& aImage);

		sync& make_memory_available_for_reading(memory_stage aMemoryToBeMadeAvailable, const image_t& aImage);

		// TODO: Support buffer memory barriers after buffer-unification:
		//sync& make_memory_available_for_writing(memory_stage aMemoryToBeMadeAvailable, const buffer_t& aBuffer);
		//sync& make_memory_available_for_reading(memory_stage aMemoryToBeMadeAvailable, const buffer_t& aBuffer);

		/**	Set the queue where the command is to be submitted to AND also where the sync will happen.
		 */
		sync& on_queue(std::reference_wrapper<device_queue> aQueue);

		/**	Target queue which shall own buffers and images that occur
		 */
		sync& then_transfer_to(std::reference_wrapper<device_queue> aQueue);

		/**	Establish an (optional) handler which can be used to alter the parameters passed to a pipeline barrier
		 */
		sync& set_pipeline_barrier_parameters_alteration_handler(std::function<void(pipeline_barrier_parameters&)> aHandler);

		/** Determine the fundamental sync approach configured in this `sync`. */
		sync_type get_sync_type() const;
		
		/** Queue which the command and sync will be submitted to. */
		std::reference_wrapper<device_queue> queue_to_use() const;

		/** Queue, which buffer and image ownership shall be transferred to. */
		std::reference_wrapper<device_queue> queue_to_transfer_to() const;

		/** Returns true if the submit-queue is the same as the ownership-queue. */
		bool queues_are_the_same() const;

		void establish_barrier_if_applicable(command_buffer& aCommandBuffer, vk::PipelineStageFlags aSourcePipelineStage) const;
		
		void submit_and_sync(command_buffer aCommandBuffer);

		
	private:
		std::function<void(owning_resource<semaphore_t>)> mSemaphoreHandler;
		std::vector<semaphore> mWaitSemaphores;
		std::function<void(command_buffer)> mCommandBufferLifetimeHandler;
		std::optional<std::reference_wrapper<device_queue>> mQueueToUse;
		std::optional<std::reference_wrapper<device_queue>> mQueueToTransferOwnershipTo;
		std::optional<vk::PipelineStageFlags> mDstStage;
		std::function<void(pipeline_barrier_parameters&)> mAlterPipelineBarrierParametersHandler;
	};
}
