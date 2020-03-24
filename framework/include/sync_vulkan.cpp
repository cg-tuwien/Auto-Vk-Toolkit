#include <cg_base.hpp>

namespace cgb
{
	sync::sync(sync&& aOther) noexcept
		: mNoSyncRequired{ std::move(aOther.mNoSyncRequired) }
		, mSemaphoreLifetimeHandler{ std::move(aOther.mSemaphoreLifetimeHandler) }
		, mWaitBeforeSemaphores{ std::move(aOther.mWaitBeforeSemaphores) }
		, mCommandBufferRefOrLifetimeHandler{ std::move(aOther.mCommandBufferRefOrLifetimeHandler) }
		, mCommandBuffer{ std::move(aOther.mCommandBuffer) }
		, mEstablishBarrierBeforeOperationCallback{ std::move(aOther.mEstablishBarrierBeforeOperationCallback) }
		, mEstablishBarrierAfterOperationCallback{ std::move(aOther.mEstablishBarrierAfterOperationCallback) }
		, mQueueToUse{ std::move(aOther.mQueueToUse) }
	{
		aOther.mNoSyncRequired = true;
		aOther.mSemaphoreLifetimeHandler = {};
		aOther.mWaitBeforeSemaphores.clear();
		aOther.mCommandBufferRefOrLifetimeHandler = {};
		aOther.mCommandBuffer.reset();
		aOther.mEstablishBarrierBeforeOperationCallback = {};
		aOther.mEstablishBarrierAfterOperationCallback = {};
		aOther.mQueueToUse.reset();
	}

	sync& sync::operator=(sync&& aOther) noexcept
	{
		mNoSyncRequired = std::move(aOther.mNoSyncRequired);
		mSemaphoreLifetimeHandler = std::move(aOther.mSemaphoreLifetimeHandler);
		mWaitBeforeSemaphores = std::move(aOther.mWaitBeforeSemaphores);
		mCommandBufferRefOrLifetimeHandler = std::move(aOther.mCommandBufferRefOrLifetimeHandler);
		mCommandBuffer = std::move(aOther.mCommandBuffer);
		mEstablishBarrierBeforeOperationCallback = std::move(aOther.mEstablishBarrierBeforeOperationCallback);
		mEstablishBarrierAfterOperationCallback = std::move(aOther.mEstablishBarrierAfterOperationCallback);
		mQueueToUse = std::move(aOther.mQueueToUse);
		
		aOther.mNoSyncRequired = true;
		aOther.mSemaphoreLifetimeHandler = {};
		aOther.mWaitBeforeSemaphores.clear();
		aOther.mCommandBufferRefOrLifetimeHandler = {};
		aOther.mCommandBuffer.reset();
		aOther.mEstablishBarrierBeforeOperationCallback = {};
		aOther.mEstablishBarrierAfterOperationCallback = {};
		aOther.mQueueToUse.reset();

		return *this;
	}
	
	sync::~sync()
	{
		if (mCommandBuffer.has_value()) {
			LOG_ERROR("Command buffer has not been submitted but cgb::sync instance is destructed. This must be a bug.");
		}
#ifdef _DEBUG
		if (mEstablishBarrierBeforeOperationCallback) {
			LOG_DEBUG("The before-operation-barrier-callback has never been invoked for this cgb::sync instance. This can be a bug, but it can be okay as well.");
		}
		if (mEstablishBarrierBeforeOperationCallback) {
			LOG_DEBUG("The after-operation-barrier-callback has never been invoked for this cgb::sync instance. This can be a bug, but it can be okay as well.");
		}
#endif
	}
	
	sync sync::not_required()
	{
		sync result;
		result.mNoSyncRequired = true; // User explicitely stated that there's no sync required.
		return result;
	}
	
	sync sync::wait_idle()
	{
		sync result;
		return result;
	}
	
	sync sync::with_semaphores(std::function<void(semaphore)> aSignalledAfterOperation, std::vector<semaphore> aWaitBeforeOperation)
	{
		sync result;
		result.mSemaphoreLifetimeHandler = std::move(aSignalledAfterOperation);
		result.mWaitBeforeSemaphores = std::move(aWaitBeforeOperation);
		return result;
	}
	
	sync sync::with_semaphores_on_current_frame(std::vector<semaphore> aWaitBeforeOperation, cgb::window* aWindow)
	{
		sync result;
		result.mSemaphoreLifetimeHandler = [wnd = nullptr != aWindow ? aWindow : cgb::context().main_window()] (auto aSemaphore) {  
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
		result.mCommandBufferRefOrLifetimeHandler = std::move(aCommandBufferLifetimeHandler); // <-- Set the lifetime handler, not the command buffer reference.
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
		result.mCommandBufferRefOrLifetimeHandler = [wnd = nullptr != aWindow ? aWindow : cgb::context().main_window()] (auto aCmdBfr) {  
			wnd->handle_single_use_command_buffer_lifetime(std::move(aCmdBfr));
		};  // <-- Set the lifetime handler, not the command buffer reference.
		result.mEstablishBarrierAfterOperationCallback = std::move(aEstablishBarrierAfterOperation);
		result.mEstablishBarrierBeforeOperationCallback = std::move(aEstablishBarrierBeforeOperation);
		return result;
	}

	sync sync::auxiliary_with_barriers(
			sync& aMasterSync,
			std::function<void(command_buffer_t&, pipeline_stage /* destination stage */, std::optional<read_memory_access> /* destination access */)> aEstablishBarrierBeforeOperation,
			std::function<void(command_buffer_t&, pipeline_stage /* source stage */, std::optional<write_memory_access> /* source access */)> aEstablishBarrierAfterOperation
		)
	{
		// Perform some checks
		const auto stealBeforeHandlerOnDemand = is_about_to_steal_before_handler_on_demand(aEstablishBarrierBeforeOperation);
		const auto stealAfterHandlerOnDemand = is_about_to_steal_after_handler_on_demand(aEstablishBarrierAfterOperation);
		const auto stealBeforeHandlerImmediately = is_about_to_steal_before_handler_immediately(aEstablishBarrierBeforeOperation);
		const auto stealAfterHandlerImmediately = is_about_to_steal_after_handler_immediately(aEstablishBarrierAfterOperation);
		assert(2 != (static_cast<int>(stealBeforeHandlerImmediately) + static_cast<int>(stealBeforeHandlerOnDemand)));
		assert(2 != (static_cast<int>(stealAfterHandlerImmediately) + static_cast<int>(stealAfterHandlerOnDemand)));

		// Possibly steal something
		if (stealBeforeHandlerOnDemand) {
			aEstablishBarrierBeforeOperation = [&aMasterSync](command_buffer_t& cb, pipeline_stage stage, std::optional<read_memory_access> access) {
				// Execute and invalidate:
				auto handler = std::move(aMasterSync.mEstablishBarrierBeforeOperationCallback);
				aMasterSync.mEstablishBarrierBeforeOperationCallback = {};
				if (handler) {
					handler(cb, stage, access);
				}
			};
		}
		else if (stealBeforeHandlerImmediately) {
			aEstablishBarrierBeforeOperation = std::move(aMasterSync.mEstablishBarrierBeforeOperationCallback);
			aMasterSync.mEstablishBarrierBeforeOperationCallback = {};
		}
		
		if (stealAfterHandlerOnDemand) {
			aEstablishBarrierAfterOperation = [&aMasterSync](command_buffer_t& cb, pipeline_stage stage, std::optional<write_memory_access> access) {
				// Execute and invalidate:
				auto handler = std::move(aMasterSync.mEstablishBarrierAfterOperationCallback);
				aMasterSync.mEstablishBarrierAfterOperationCallback = {};
				if (handler) {
					handler(cb, stage, access);
				}
			};
		}
		else if (stealAfterHandlerImmediately) {
			aEstablishBarrierAfterOperation = std::move(aMasterSync.mEstablishBarrierAfterOperationCallback);
			aMasterSync.mEstablishBarrierAfterOperationCallback = {};
		}


		// Prepare a shiny new sync instance
		sync result;
		
		switch (aMasterSync.get_sync_type()) {
		case sync_type::not_required:
			result = std::move(sync::not_required());
		case sync_type::via_wait_idle:
			result = std::move(sync::wait_idle());
		case sync_type::via_semaphore:
			//result = std::move(sync::with_barriers([&aMasterSync](command_buffer aCbToHandle) {
			//		assert(aMasterSync.mSemaphoreLifetimeHandler);
			//		// Let's do something sneaky => swap out master's semaphore handler:
			//		auto tmp = std::move(aMasterSync.mSemaphoreLifetimeHandler);
			//		aMasterSync.mSemaphoreLifetimeHandler = [&aCbToHandle, lMasterHandler{std::move(tmp)}](semaphore aSemaphore) {
			//			aSemaphore->set_custom_deleter([lCmdBfr{std::move(aCbToHandle)}](){});
			//			// Call the original handler:
			//			lMasterHandler(std::move(aSemaphore));
			//		};
			//	},
			//	std::move(aEstablishBarrierBeforeOperation),
			//	std::move(aEstablishBarrierAfterOperation)
			//));

			// TODO: Semaphore and barrier options are the very same from this point on, are they?!
			
			//result.mCommandBufferRefOrLifetimeHandler = std::ref(aMasterCommandBuffer); // <-- Set the command buffer reference, not the lifetime handler
			//result.mEstablishBarrierAfterOperationCallback = std::move(aEstablishBarrierAfterOperation);
			//result.mEstablishBarrierBeforeOperationCallback = std::move(aEstablishBarrierBeforeOperation);
			//break;
		case sync_type::via_barrier:
			//result = std::move(sync::with_barriers([&aMasterSync](command_buffer aCbToHandle){
			//		assert(aMasterSync.mCommandBufferLifetimeHandler);
			//		// Let's do something sneaky => swap out master's command buffer handler:
			//		auto tmp = std::move(aMasterSync.mCommandBufferLifetimeHandler);
			//		aMasterSync.mCommandBufferLifetimeHandler = [&aCbToHandle, lMasterHandler{std::move(tmp)}](command_buffer aMasterCb){
			//			aMasterCb->set_custom_deleter([lCmdBfr{std::move(aCbToHandle)}]() {});
			//			// Call the original handler:
			//			lMasterHandler(std::move(aMasterCb));
			//		};
			//	},
			//	std::move(aEstablishBarrierBeforeOperation),
			//	std::move(aEstablishBarrierAfterOperation)
			//));
			result.mCommandBufferRefOrLifetimeHandler = std::ref(aMasterSync.get_or_create_command_buffer()); // <-- Set the command buffer reference, not the lifetime handler
			result.mEstablishBarrierAfterOperationCallback = std::move(aEstablishBarrierAfterOperation);
			result.mEstablishBarrierBeforeOperationCallback = std::move(aEstablishBarrierBeforeOperation);
			break;
		default:
			assert(false); throw std::logic_error("How did we end up here?");
		}

		result.on_queue(aMasterSync.queue_to_use());
		return result;
	}

	sync& sync::on_queue(std::reference_wrapper<device_queue> aQueue)
	{
		mQueueToUse = aQueue;
		return *this;
	}
	
	sync::sync_type sync::get_sync_type() const
	{
		if (mSemaphoreLifetimeHandler) {
			return sync_type::via_semaphore;
		}
		if (!std::holds_alternative<std::monostate>(mCommandBufferRefOrLifetimeHandler)) {
			return sync_type::via_barrier;
		}
		return mNoSyncRequired ? sync_type::not_required : sync_type::via_wait_idle;
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

	command_buffer_t& sync::get_or_create_command_buffer()
	{
		if (std::holds_alternative<std::reference_wrapper<command_buffer_t>>(mCommandBufferRefOrLifetimeHandler)) {
			return std::get<std::reference_wrapper<command_buffer_t>>(mCommandBufferRefOrLifetimeHandler).get();
		}

		if (!mCommandBuffer.has_value()) {
			mCommandBuffer = queue_to_use().get().create_single_use_command_buffer();
			mCommandBuffer.value()->begin_recording(); // Immediately start recording
		}
		return mCommandBuffer.value();
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

	void sync::set_queue_hint(std::reference_wrapper<device_queue> aQueueRecommendation)
	{
		if (!mQueueToUse.has_value()) {
			mQueueToUse = aQueueRecommendation;
		}
	}
	
	void sync::establish_barrier_before_the_operation(pipeline_stage aDestinationPipelineStages, std::optional<read_memory_access> aDestinationMemoryStages)
	{
		if (!mEstablishBarrierBeforeOperationCallback) {
			return; // nothing to do here
		}
		mEstablishBarrierBeforeOperationCallback(get_or_create_command_buffer(), aDestinationPipelineStages, aDestinationMemoryStages);
		mEstablishBarrierBeforeOperationCallback = {};
	}

	void sync::establish_barrier_after_the_operation(pipeline_stage aSourcePipelineStages, std::optional<write_memory_access> aSourceMemoryStages)
	{
		if (!mEstablishBarrierAfterOperationCallback) {
			return; // nothing to do here
		}
		mEstablishBarrierAfterOperationCallback(get_or_create_command_buffer(), aSourcePipelineStages, aSourceMemoryStages);
		mEstablishBarrierAfterOperationCallback = {};
	}

	void sync::submit_and_sync()
	{
		device_queue& queue = queue_to_use();
		auto syncType = get_sync_type();
		switch (syncType) {
		case sync_type::via_semaphore:
			{
				assert(mSemaphoreLifetimeHandler);
				assert(mCommandBuffer.has_value());
				mCommandBuffer.value()->establish_global_memory_barrier(
					pipeline_stage::all_commands, 
					pipeline_stage::all_commands, 
					std::optional<memory_access>{memory_access::any_access}, 
					std::optional<memory_access>{memory_access::any_access});
				mCommandBuffer.value()->end_recording();	// What started in get_or_create_command_buffer() ends here.
				auto sema = queue.submit_and_handle_with_semaphore(std::move(mCommandBuffer.value()), std::move(mWaitBeforeSemaphores));
				mSemaphoreLifetimeHandler(std::move(sema)); // Transfer ownership and be done with it
				mCommandBuffer.reset();						// Command buffer has been moved from. It's gone.
				mWaitBeforeSemaphores.clear();				// Never ever use them again (they have been moved from)
			}
			break;
		case sync_type::via_barrier:
			assert(!std::holds_alternative<std::monostate>(mCommandBufferRefOrLifetimeHandler));
			if (std::holds_alternative<std::function<void(command_buffer)>>(mCommandBufferRefOrLifetimeHandler)) {
				assert(mCommandBuffer.has_value());
				mCommandBuffer.value()->end_recording();	// What started in get_or_create_command_buffer() ends here.
				queue.submit(mCommandBuffer.value());
				std::get<std::function<void(command_buffer)>>(mCommandBufferRefOrLifetimeHandler)(std::move(mCommandBuffer.value())); // Transfer ownership and be done with it.
				mCommandBuffer.reset();						// Command buffer has been moved from. It's gone.
			}
			else { // Must mean that we are an auxiliary sync handler
				assert(std::holds_alternative<std::reference_wrapper<command_buffer_t>>(mCommandBufferRefOrLifetimeHandler));
				// ... that means: Nothing to do here. Master sync will submit the command buffer.
			}
			break;
		case sync_type::via_wait_idle:
			assert(mCommandBuffer.has_value());
			mCommandBuffer.value()->end_recording();		// What started in get_or_create_command_buffer() ends here.
			queue.submit(mCommandBuffer.value());
			LOG_WARNING(fmt::format("Performing waitIdle on queue {} in order to sync because no other type of handler is present.", queue_to_use().get().queue_index()));
			queue_to_use().get().handle().waitIdle();
			mCommandBuffer.reset();							// Command buffer is fully handled after waitIdle() and can be destroyed.
			mNoSyncRequired = true; // No further sync required
			break;
		case sync_type::not_required:
			assert(false);
			throw std::runtime_error("You were wrong with your assumption that there was no sync required! => Provide a concrete sync strategy!");
		default:
			assert(false);
			throw std::logic_error("unknown syncType");
		}
	}

}
