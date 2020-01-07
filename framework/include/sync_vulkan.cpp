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
	
	sync sync::with_barrier(std::function<void(command_buffer)> aCommandBufferLifetimeHandler)
	{
		sync result;
		result.mCommandBufferLifetimeHandler = std::move(aCommandBufferLifetimeHandler);
		return result;
	}
	
	sync sync::with_barrier_on_current_frame(cgb::window* aWindow)
	{
		sync result;
		result.mCommandBufferLifetimeHandler = [wnd = nullptr != aWindow ? aWindow : cgb::context().main_window()] (auto aCmdBfr) {  
			wnd->handle_single_use_command_buffer_lifetime(std::move(aCmdBfr));
		};
		return result;
	}

	sync& sync::before_destination_stage(vk::PipelineStageFlags aStageWhichHasToWait)
	{
		set_destination_stage(aStageWhichHasToWait);
		return *this;
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

	sync::sync_type sync::get_sync_type() const
	{
		if (mSemaphoreHandler) return sync_type::via_semaphore;
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

	void sync::establish_barrier_if_applicable(command_buffer& aCommandBuffer, vk::PipelineStageFlags aSourcePipelineStage) const
	{
		if (sync_type::via_barrier != get_sync_type()) {
			return; // nothing to do in this case
		}
		aCommandBuffer->handle().pipelineBarrier(
			aSourcePipelineStage, mDstStage.value_or(vk::PipelineStageFlagBits::eAllCommands),
			{}, // TODO: Dependency flags might become relevant with framebuffers and sub-passes (e.g. like the "by region" setting which can be a huge improvement on mobile)
			{}, // TODO: Memory Barriers
			{}, // TODO: Buffer Memory Barriers
			{} // TODO: Image Memory Barriers
		);
	}
	
	void sync::submit_and_sync(command_buffer aCommandBuffer)
	{
		device_queue& queue = queue_to_use();
		auto syncType = get_sync_type();
		switch (syncType) {
		case sync_type::via_semaphore:
			{
				assert(mSemaphoreHandler);
				auto sema = queue.submit_and_handle_with_semaphore(std::move(aCommandBuffer), std::move(mWaitSemaphores));
				if (mDstStage.has_value()) {
					sema->set_semaphore_wait_stage(mDstStage.value());
				}
				mSemaphoreHandler(std::move(sema)); // Transfer ownership and be done with it
				mWaitSemaphores = {};
				mSemaphoreHandler = {};
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
