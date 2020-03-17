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
		std::vector<vk::MemoryBarrier> mMemoryBarriers;
		std::vector<vk::BufferMemoryBarrier> mBufferMemoryBarriers;
		std::vector<vk::ImageMemoryBarrier> mImageMemoryBarriers;
	};

	/**	The sync class is a fundamental part of the framework and is used wherever synchronization is or can be needed.
	 *	It allows a caller to inject a specific synchronization strategy into a particular method/function.
	 */
	class sync
	{
	public:
		enum struct sync_type { via_wait_idle, via_semaphore, via_barrier };
		
		sync() = default;
		sync(const sync&) = delete;
		sync(sync&&) noexcept = default;
		sync& operator=(const sync&) = delete;
		sync& operator=(sync&&) noexcept = default;

#pragma region static creation functions
		/**	Establish very coarse (and inefficient) synchronization by waiting for the queue to become idle before continuing.
		 */
		static sync wait_idle();

		/**	Establish semaphore-based synchronization with a custom semaphore lifetime handler.
		 *	@param	aSemaphoreHandler	A function to handle the lifetime of a created semaphore. 
		 *	@param	aWaitSemaphores		A vector of other semaphores to be waited on before executing the command.
		 */
		static sync with_semaphore(std::function<void(owning_resource<semaphore_t>)> aSemaphoreHandler, std::vector<semaphore> aWaitSemaphores = {});

		/**	Establish semaphore-based synchronization and have its lifetime handled w.r.t the window's swap chain.
		 *	@param	aWaitSemaphores		A vector of other semaphores to be waited on before executing the command.
		 *	@param	aWindow				A window, whose swap chain shall be used to handle the lifetime of the possibly emerging semaphore.
		 */
		static sync with_semaphore_on_current_frame(std::vector<semaphore> aWaitSemaphores = {}, cgb::window* aWindow = nullptr);

		/**	Establish barrier-based synchronization with a custom command buffer lifetime handler.
		 *	@param	aCommandBufferLifetimeHandler	A function to handle the lifetime of a command buffer.
		 */
		static sync with_barrier(std::function<void(command_buffer)> aCommandBufferLifetimeHandler);

		/**	Establish barrier-based synchronization with a custom command buffer lifetime handler.
		 *	@param	aCommandBufferLifetimeHandler	A function to handle the lifetime of a command buffer.
		 */
		static sync with_barrier_on_current_frame(cgb::window* aWindow = nullptr);
#pragma endregion 

#pragma region execution and memory dependencies
		/**	Establish an EXECUTION BARRIER: The specified stage has to wait on the completion of the command to be synchronized.
		 *  THIS METHOD can be used to set the destination stage of the pipeline execution barrier.
		 *  The source stage, on the other hand, will be set by the command.
		 *  @param	aStageWhichHasToWait		The destination stage, i.e. the stage which comes after the command and must wait.
		 *
		 *	This flag applies to: synchronization with semaphores, and synchronization with barriers.
		 *	It can/should be used in conjunction with a memory barrier.
		 *	TODO: ^ really?
		 */
		sync& continue_execution_in_stages(pipeline_stage aStageWhichHasToWait);

		/**	Establish a MEMORY BARRIER of write after write (WaW) type, IF the command to be synchronized does write to memory.
		 *	@param	aMemoryToBeMadeAvailable	Which kinds of memory operations to be made available before continuing execution.
		 *
		 *	This flag applies to: synchronization with barriers (not to semaphores!).
		 *	It can/should be used in conjunction with an execution barriers.
		 *	TODO: ^ really?
		 */
		sync& make_memory_available_for_writing_in_stages(memory_stage aMemoryToBeMadeAvailable);

		/**	Establish a MEMORY BARRIER of read after write (RaW) type, IF the command to be synchronized does write to memory.
		 *	@param	aMemoryToBeMadeAvailable	Which kinds of memory operations to be made available before continuing execution.
		 *
		 *	This flag applies to: synchronization with barriers (not to semaphores!).
		 *	It can/should be used in conjunction with an execution barriers.
		 *	TODO: ^ really?
		 */
		sync& make_memory_available_for_reading_in_stages(memory_stage aMemoryToBeMadeAvailable);

		/**	Establish an IMAGE MEMORY BARRIER of write after write (WaW) type.
		 *	The image's layout is automatically transitioned if its current layout does not match its target layout.
		 *	@param	aMemoryToBeMadeAvailable	Which kinds of memory operations to be made available before continuing execution.
		 *
		 *	This flag applies to: synchronization with barriers (not to semaphores!).
		 *	It can/should be used in conjunction with an execution barriers.
		 *	TODO: ^ really?
		 */
		sync& make_image_available_for_writing_in_stages(const image_t& aImage, memory_stage aMemoryToBeMadeAvailable = memory_stage::any_access);

		/**	Establish an IMAGE MEMORY BARRIER of read after write (RaW) type.
		 *	The image's layout is automatically transitioned if its current layout does not match its target layout.
		 *	@param	aMemoryToBeMadeAvailable	Which kinds of memory operations to be made available before continuing execution.
		 *
		 *	This flag applies to: synchronization with barriers (not to semaphores!).
		 *	It can/should be used in conjunction with an execution barriers.
		 *	TODO: ^ really?
		 */
		sync& make_image_available_for_reading_in_stages(const image_t& aImage, memory_stage aMemoryToBeMadeAvailable = memory_stage::any_access);
		
		// TODO: Support buffer memory barriers after buffer-unification:
		//sync& make_memory_available_for_writing(memory_stage aMemoryToBeMadeAvailable, const buffer_t& aBuffer);
		//sync& make_memory_available_for_reading(memory_stage aMemoryToBeMadeAvailable, const buffer_t& aBuffer);
#pragma endregion 

#pragma region ownership-related settings
		/**	Set the queue where the command is to be submitted to AND also where the sync will happen.
		 */
		sync& on_queue(std::reference_wrapper<device_queue> aQueue);

		/**	Target queue which shall own buffers and images that occur
		 */
		sync& then_transfer_to(std::reference_wrapper<device_queue> aQueue);

		/**	Establish an (optional) handler which can be used to alter the parameters passed to a pipeline barrier
		 */
		sync& set_pipeline_barrier_parameters_alteration_handler(std::function<void(pipeline_barrier_parameters&)> aHandler);
#pragma endregion 

#pragma region getters 
		/** Determine the fundamental sync approach configured in this `sync`. */
		sync_type get_sync_type() const;
		
		/** Queue which the command and sync will be submitted to. */
		std::reference_wrapper<device_queue> queue_to_use() const;

		/** Queue, which buffer and image ownership shall be transferred to. */
		std::reference_wrapper<device_queue> queue_to_transfer_to() const;

		/** Returns true if the submit-queue is the same as the ownership-queue. */
		bool queues_are_the_same() const;
#pragma endregion 

#pragma region essential functions which establish the actual sync. Used by the framework internally.
		/**	Establish a barrier on the given command buffer according to the parameters that have been configured.
		 *	This method is intended not to be used by framework-consuming code, but by the framework-internals.
		 *	This method has an important secondary role (you could call it "side effect"): It stores the source pipelie stage,
		 *	which is not only relevant for barrier-based synchronization, but also for semaphore-based synchronization.
		 *	I.e. even if you only use semaphore-based synchronization (which shouldn't be the case in general), call this function before calling `submit_and_sync`!
		 *	
		 *	@param	aCommandBuffer				The command buffer where to establish the barrier on.
		 *	@param	aSourcePipelineStages		The pipeline stage(s) where the operation represented by aCommandBuffer performs memory write.
		 *										The point is that subsequent commands do not start executing before the given pipeline stage(s) have been reached.
		 *										If the command does not perform memory writes, this parameter can be left unset.
		 *										The operation =^= source stage.
		 *	@param	aSourceMemoryStages			The memory stage(s) where the operation represented by aCommandBuffer performs memory write.
		 *										The point is that the write of the operation shall be made available to subsequent commands.
		 *										If the command does not perform memory writes, this parameter can be left unset.
		 *										The operation =^= source stage.
		 */
		void set_sync_stages_and_establish_barrier(command_buffer& aCommandBuffer, std::optional<pipeline_stage> aSourcePipelineStages, std::optional<memory_stage> aSourceMemoryStages);

		/**	Submit the command buffer and engage sync!
		 *	This method is intended not to be used by framework-consuming code, but by the framework-internals.
		 *	Whichever synchronization strategy has been configured for this `cgb::sync`, it will be executed here
		 *	(i.e. waiting idle, establishing a barrier, or creating a semaphore).
		 *
		 *	@param	aCommandBuffer				Hand over ownership of a command buffer in a "fire and forget"-manner from this method call on.
		 *										The command buffer will be submitted to a queue (whichever queue is configured in this `cgb::sync`)
		 */
		void submit_and_sync(command_buffer aCommandBuffer);
#pragma endregion
		
	private:
		pipeline_barrier_parameters mBarrierParams;
		std::function<void(owning_resource<semaphore_t>)> mSemaphoreHandler;
		std::vector<semaphore> mWaitSemaphores;
		std::function<void(command_buffer)> mCommandBufferLifetimeHandler;
		std::optional<std::reference_wrapper<device_queue>> mQueueToUse;
		std::optional<std::reference_wrapper<device_queue>> mQueueToTransferOwnershipTo;
		std::function<void(pipeline_barrier_parameters&)> mAlterPipelineBarrierParametersHandler;
	};
}
