#include <cg_base.hpp>

namespace cgb
{
	sync sync::wait_idle()
	{
		sync result;
		return result;
	}
	
	sync sync::with_semaphore(std::function<void(owning_resource<semaphore_t>)> aSemaphoreHandler, std::vector<semaphore> aWaitSemaphores)
	{
		sync result;
		result.mSemaphoreHandler = std::move(aSemaphoreHandler);
		result.mWaitSemaphores = std::move(aWaitSemaphores);
		return result;
	}
	
	sync sync::with_semaphore_on_current_frame(std::vector<semaphore> aWaitSemaphores, cgb::window* aWindow)
	{
		sync result;
		result.mSemaphoreHandler = [wnd = nullptr != aWindow ? aWindow : cgb::context().main_window()] (auto aSemaphore) {  
			wnd->set_extra_semaphore_dependency(std::move(aSemaphore));
		};
		result.mWaitSemaphores = std::move(aWaitSemaphores);
		return result;
	}
	
	sync sync::with_barrier(std::function<void(command_buffer&)> aEstablishBarrierCallback, std::function<void(command_buffer)> aCommandBufferLifetimeHandler)
	{
		sync result;
		result.mEstablishBarrierCallback = std::move(aEstablishBarrierCallback);
		result.mCommandBufferLifetimeHandler = std::move(aCommandBufferLifetimeHandler);
		return result;
	}
	
	sync sync::with_barrier_on_current_frame(std::function<void(command_buffer&)> aEstablishBarrierCallback, cgb::window* aWindow)
	{
		sync result;
		result.mEstablishBarrierCallback = std::move(aEstablishBarrierCallback);
		result.mCommandBufferLifetimeHandler = [wnd = nullptr != aWindow ? aWindow : cgb::context().main_window()] (auto aCmdBfr) {  
			wnd->handle_single_use_command_buffer_lifetime(std::move(aCmdBfr));
		};
		return result;
	}

	sync& sync::on_queue(std::reference_wrapper<device_queue> aQueue)
	{
		mQueueToUse = aQueue;
		return *this;
	}
	
	sync& sync::then_transfer_to(std::reference_wrapper<device_queue> aQueue)
	{
		mQueueToTransferOwnershipTo = aQueue;
		return *this;
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
	
	std::reference_wrapper<device_queue> sync::queue_to_transfer_to() const
	{
		return mQueueToTransferOwnershipTo.value_or(queue_to_use());
	}
	
	bool sync::queues_are_the_same() const
	{
		device_queue& q0 = queue_to_use();
		device_queue& q1 = queue_to_transfer_to();
		return q0 == q1;
	}

	void sync::handle_before_end_of_recording(command_buffer& aCommandBuffer) const
	{
		if (mEstablishBarrierCallback) {
			mEstablishBarrierCallback(aCommandBuffer);
		}
	}
	
	void sync::handle_after_end_of_recording(command_buffer aCommandBuffer)
	{
		device_queue& queue = queue_to_use();
		if (mSemaphoreHandler) {
			auto sema = queue.submit_and_handle_with_semaphore(std::move(aCommandBuffer), std::move(mWaitSemaphores));
			if (mDstStage.has_value()) {
				sema->set_semaphore_wait_stage(mDstStage.value());
				// TODO: Also use mSrcStage, mSrcAccess, and mDstAccess?
			}
			handle_semaphore(std::move(sema), std::move(mSemaphoreHandler));
			mWaitSemaphores = {};
			mSemaphoreHandler = {};
		}
		else {
			queue.submit(aCommandBuffer);
			assert(aCommandBuffer->state() == command_buffer_state::submitted);
			if (mCommandBufferLifetimeHandler) {
				mCommandBufferLifetimeHandler(std::move(aCommandBuffer));
			}
			else {
				queue_to_use().get().handle().waitIdle();
			}
		}
	}
}
