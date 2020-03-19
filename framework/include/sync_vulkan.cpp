#include <cg_base.hpp>

namespace cgb
{
	sync sync::wait_idle()
	{
		sync result;
		return result;
	}
	
	sync sync::with_semaphores(std::function<void(semaphore)> aSignalledAfterOperation, std::vector<semaphore> aWaitBeforeOperation)
	{
		sync result;
		result.mSemaphoreSignalAfterAndLifetimeHandler = std::move(aSignalledAfterOperation);
		result.mWaitBeforeSemaphores = std::move(aWaitBeforeOperation);
		return result;
	}
	
	sync sync::with_semaphores_on_current_frame(std::vector<semaphore> aWaitBeforeOperation, cgb::window* aWindow)
	{
		sync result;
		result.mSemaphoreSignalAfterAndLifetimeHandler = [wnd = nullptr != aWindow ? aWindow : cgb::context().main_window()] (auto aSemaphore) {  
			wnd->set_extra_semaphore_dependency(std::move(aSemaphore));
		};
		result.mWaitBeforeSemaphores = std::move(aWaitBeforeOperation);
		return result;
	}
	
	sync sync::with_barriers(
			std::function<void(command_buffer)> aCommandBufferLifetimeHandler,
			std::function<void(command_buffer_t&, pipeline_stage /* destination stage */, std::optional<read_memory_access> /* destination access */)> aEstablishBarrierBeforeOperation,
			std::function<void(command_buffer_t&, pipeline_stage /* source stage */, std::optional<write_memory_access> /* source access */)> aEstablishBarrierAfterOperation
		)
	{
		sync result;
		result.mCommandBufferLifetimeHandler = std::move(aCommandBufferLifetimeHandler);
		result.mEstablishBarrierAfterOperationCallback = std::move(aEstablishBarrierAfterOperation);
		result.mEstablishBarrierBeforeOperationCallback = std::move(aEstablishBarrierBeforeOperation);
		return result;
	}
	
	sync sync::with_barriers_on_current_frame(
			std::function<void(command_buffer_t&, pipeline_stage /* destination stage */, std::optional<read_memory_access> /* destination access */)> aEstablishBarrierBeforeOperation,
			std::function<void(command_buffer_t&, pipeline_stage /* source stage */, std::optional<write_memory_access> /* source access */)> aEstablishBarrierAfterOperation,
			cgb::window* aWindow
		)
	{
		sync result;
		result.mCommandBufferLifetimeHandler = [wnd = nullptr != aWindow ? aWindow : cgb::context().main_window()] (auto aCmdBfr) {  
			wnd->handle_single_use_command_buffer_lifetime(std::move(aCmdBfr));
		};
		result.mEstablishBarrierAfterOperationCallback = std::move(aEstablishBarrierAfterOperation);
		result.mEstablishBarrierBeforeOperationCallback = std::move(aEstablishBarrierBeforeOperation);
		return result;
	}

	sync sync::with_barriers_subordinate(
			sync& aMasterSync,
			std::function<void(command_buffer_t&, pipeline_stage /* destination stage */, std::optional<read_memory_access> /* destination access */)> aEstablishBarrierBeforeOperation,
			std::function<void(command_buffer_t&, pipeline_stage /* source stage */, std::optional<write_memory_access> /* source access */)> aEstablishBarrierAfterOperation,
			bool aStealMastersBeforeHandler
		)
	{
		assert(aStealMastersBeforeHandler != static_cast<bool>(aEstablishBarrierBeforeOperation));
		if (aStealMastersBeforeHandler) {
			aEstablishBarrierBeforeOperation = std::move(aMasterSync.mEstablishBarrierBeforeOperationCallback);
			aMasterSync.mEstablishBarrierBeforeOperationCallback = {};
		}
		
		switch (aMasterSync.get_sync_type()) {
		case sync_type::via_wait_idle:
			return sync::wait_idle(); // <-- Nothing really matters
		case sync_type::via_semaphore:
			return sync::with_barriers([&aMasterSync](command_buffer aCbToHandle) {
					assert(aMasterSync.mSemaphoreSignalAfterAndLifetimeHandler);
					// Let's do something sneaky => swap out master's semaphore handler:
					auto tmp = std::move(aMasterSync.mSemaphoreSignalAfterAndLifetimeHandler);
					aMasterSync.mSemaphoreSignalAfterAndLifetimeHandler = [&aCbToHandle, lMasterHandler{std::move(tmp)}](semaphore aSemaphore) {
						aSemaphore->set_custom_deleter([lCmdBfr{std::move(aCbToHandle)}](){});
						// Call the original handler:
						lMasterHandler(std::move(aSemaphore));
					};
				},
				std::move(aEstablishBarrierBeforeOperation),
				std::move(aEstablishBarrierAfterOperation)
			);
		case sync_type::via_barrier:
			return sync::with_barriers([&aMasterSync](command_buffer aCbToHandle){
					assert(aMasterSync.mCommandBufferLifetimeHandler);
					// Let's do something sneaky => swap out master's command buffer handler:
					auto tmp = std::move(aMasterSync.mCommandBufferLifetimeHandler);
					aMasterSync.mCommandBufferLifetimeHandler = [&aCbToHandle, lMasterHandler{std::move(tmp)}](command_buffer aMasterCb){
						aMasterCb->set_custom_deleter([lCmdBfr{std::move(aCbToHandle)}]() {});
						// Call the original handler:
						lMasterHandler(std::move(aMasterCb));
					};
				},
				std::move(aEstablishBarrierBeforeOperation),
				std::move(aEstablishBarrierAfterOperation)
			);
		default:
			assert(false); throw std::logic_error("How did we end up here?");
		}
	}

	sync& sync::on_queue(std::reference_wrapper<device_queue> aQueue)
	{
		mQueueToUse = aQueue;
		return *this;
	}
	
	sync::sync_type sync::get_sync_type() const
	{
		if (mSemaphoreSignalAfterAndLifetimeHandler) return sync_type::via_semaphore;
		if (mCommandBufferLifetimeHandler) return sync_type::via_barrier;
		return sync_type::via_wait_idle();
	}
	
	std::reference_wrapper<device_queue> sync::queue_to_use() const
	{
#ifdef _DEBUG
		if (!mQueueToUse.has_value()) {
			LOG_WARNING("No queue specified => will submit to the graphics queue. HTH.");
		}
#endif
		return mQueueToUse.value_or(cgb::context().graphics_queue());
	}
	
	//std::reference_wrapper<device_queue> sync::queue_to_transfer_to() const
	//{
	//	return mQueueToTransferOwnershipTo.value_or(queue_to_use());
	//}
	//
	//bool sync::queues_are_the_same() const
	//{
	//	device_queue& q0 = queue_to_use();
	//	device_queue& q1 = queue_to_transfer_to();
	//	return q0 == q1;
	//}

	void sync::establish_barrier_before_the_operation(command_buffer& aCommandBuffer, pipeline_stage aDestinationPipelineStages, std::optional<read_memory_access> aDestinationMemoryStages) const
	{
		if (!mEstablishBarrierBeforeOperationCallback) {
			return; // nothing to do here
		}
		mEstablishBarrierBeforeOperationCallback(aCommandBuffer, aDestinationPipelineStages, aDestinationMemoryStages);
	}

	void sync::establish_barrier_after_the_operation(command_buffer& aCommandBuffer, pipeline_stage aSourcePipelineStages, std::optional<write_memory_access> aSourceMemoryStages) const
	{
		if (!mEstablishBarrierAfterOperationCallback) {
			return; // nothing to do here
		}
		mEstablishBarrierAfterOperationCallback(aCommandBuffer, aSourcePipelineStages, aSourceMemoryStages);
	}

	void sync::submit_and_sync(command_buffer aCommandBuffer)
	{
		device_queue& queue = queue_to_use();
		auto syncType = get_sync_type();
		switch (syncType) {
		case sync_type::via_semaphore:
			{
				assert(mSemaphoreSignalAfterAndLifetimeHandler);
				auto sema = queue.submit_and_handle_with_semaphore(std::move(aCommandBuffer), std::move(mWaitBeforeSemaphores));
				mSemaphoreSignalAfterAndLifetimeHandler(std::move(sema)); // Transfer ownership and be done with it
				mWaitBeforeSemaphores = {};
				mSemaphoreSignalAfterAndLifetimeHandler = {};
			}
			break;
		case sync_type::via_barrier:
			assert(mCommandBufferLifetimeHandler);
			queue.submit(aCommandBuffer);
			mCommandBufferLifetimeHandler(std::move(aCommandBuffer));
			mCommandBufferLifetimeHandler = {};
			break;
		case sync_type::via_wait_idle:
			queue.submit(aCommandBuffer);
			LOG_WARNING(fmt::format("Performing waitIdle on queue {} in order to sync because no other type of handler is present.", queue_to_use().get().queue_index()));
			queue_to_use().get().handle().waitIdle();
			break;
		default:
			throw std::logic_error("unknown syncType");
		}
	}
}
