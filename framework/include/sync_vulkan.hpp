#pragma once

namespace cgb
{
	// Forward-declare the device-queue
	class device_queue;
	class window;

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
		 *	@param	aSignalledAfterOperation	A function to handle the lifetime of a created semaphore. 
		 *	@param	aWaitBeforeOperation		A vector of other semaphores to be waited on before executing the command.
		 */
		static sync with_semaphores(std::function<void(semaphore)> aSignalledAfterOperation, std::vector<semaphore> aWaitBeforeOperation = {});

		/**	Establish semaphore-based synchronization and have its lifetime handled w.r.t the window's swap chain.
		 *	@param	aWaitBeforeOperation		A vector of other semaphores to be waited on before executing the command.
		 *	@param	aWindow				A window, whose swap chain shall be used to handle the lifetime of the possibly emerging semaphore.
		 */
		static sync with_semaphores_on_current_frame(std::vector<semaphore> aWaitBeforeOperation = {}, cgb::window* aWindow = nullptr);

		/**	Establish barrier-based synchronization with a custom command buffer lifetime handler.
		 *	@param	aCommandBufferLifetimeHandler	A function to handle the lifetime of a command buffer.
		 */
		static sync with_barriers(
			std::function<void(command_buffer)> aCommandBufferLifetimeHandler,
			std::function<void(command_buffer_t&, pipeline_stage /* source stage */,	  std::optional<memory_access> /* source access */)> aEstablishBarrierAfterOperation,
			std::function<void(command_buffer_t&, pipeline_stage /* destination stage */, std::optional<memory_access> /* destination access */)> aEstablishBarrierBeforeOperation = {}
		);

		/**	Establish barrier-based synchronization with a custom command buffer lifetime handler.
		 *	@param	aCommandBufferLifetimeHandler	A function to handle the lifetime of a command buffer.
		 */
		static sync with_barriers_on_current_frame(
			std::function<void(command_buffer_t&, pipeline_stage /* source stage */,	  std::optional<memory_access> /* source access */)> aEstablishBarrierAfterOperation,
			std::function<void(command_buffer_t&, pipeline_stage /* destination stage */, std::optional<memory_access> /* destination access */)> aEstablishBarrierBeforeOperation = {},
			cgb::window* aWindow = nullptr
		);

		/**	Establish barrier-based synchronization for a command which is subordinate to a
		 *	"master"-sync handler. The master handler is usually provided by the user and this
		 *	method is used to create sync objects which go along with the master sync, i.e.,
		 *	lifetime of subordinate operations' command buffers are handled along with the
		 *	master handler.
		 *
		 *	@param	aMasterSync		Master sync handler which is being modified by this method
		 *							in order to also handle lifetime of subordinate command buffers.
		 */
		static sync with_barrier_subordinate(
			sync& aMasterSync,
			std::function<void(command_buffer_t&, pipeline_stage /* source stage */, std::optional<memory_access> /* source access */)> aEstablishBarrierAfterOperation,
			std::function<void(command_buffer_t&, pipeline_stage /* destination stage */, std::optional<memory_access> /* destination access */)> aEstablishBarrierBeforeOperation = {},
			bool aStealMastersBeforeHandler = false
		);
#pragma endregion 

#pragma region ownership-related settings
		/**	Set the queue where the command is to be submitted to AND also where the sync will happen.
		 */
		sync& on_queue(std::reference_wrapper<device_queue> aQueue);
#pragma endregion 

#pragma region getters 
		/** Determine the fundamental sync approach configured in this `sync`. */
		sync_type get_sync_type() const;
		
		/** Queue which the command and sync will be submitted to. */
		std::reference_wrapper<device_queue> queue_to_use() const;
#pragma endregion 

#pragma region essential functions which establish the actual sync. Used by the framework internally.
		void establish_barrier_before_the_operation(command_buffer& aCommandBuffer, pipeline_stage aDestinationPipelineStages, std::optional<memory_access> aDestinationMemoryStages) const;
		void establish_barrier_after_the_operation(command_buffer& aCommandBuffer, pipeline_stage aSourcePipelineStages, std::optional<memory_access> aSourceMemoryStages) const;

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
		std::function<void(semaphore)> mSemaphoreSignalAfterAndLifetimeHandler;
		std::vector<semaphore> mWaitBeforeSemaphores;
		std::function<void(command_buffer)> mCommandBufferLifetimeHandler;
		std::function<void(command_buffer_t&, pipeline_stage /* source stage */,	  std::optional<memory_access> /* source access */)>	  mEstablishBarrierAfterOperationCallback;
		std::function<void(command_buffer_t&, pipeline_stage /* destination stage */, std::optional<memory_access> /* destination access */)> mEstablishBarrierBeforeOperationCallback;
		std::optional<std::reference_wrapper<device_queue>> mQueueToUse;
	};
}
