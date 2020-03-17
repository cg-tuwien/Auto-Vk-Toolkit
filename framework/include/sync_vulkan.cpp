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

	sync& sync::continue_execution_in_stages(pipeline_stage aStageWhichHasToWait)
	{
		mBarrierParams.mDstStageMask = to_vk_pipeline_stage_flags(aStageWhichHasToWait);
		return *this;
	}
	
	sync& sync::make_memory_available_for_writing_in_stages(memory_stage aMemoryToBeMadeAvailable)
	{
		mBarrierParams.mMemoryBarriers.emplace_back(
			vk::AccessFlags{} /* <-- to be set by the command */, 
			to_vk_access_flags_for_writing(aMemoryToBeMadeAvailable)
		);
		return *this;
	}

	sync& sync::make_memory_available_for_reading_in_stages(memory_stage aMemoryToBeMadeAvailable)
	{
		mBarrierParams.mMemoryBarriers.emplace_back(
			vk::AccessFlags{}, // <-- to be set by the command 
			to_vk_access_flags_for_reading(aMemoryToBeMadeAvailable)
		);
		return *this;
	}

	sync& sync::make_image_available_for_writing_in_stages(const image_t& aImage, memory_stage aMemoryToBeMadeAvailable)
	{
		mBarrierParams.mImageMemoryBarriers.emplace_back(
			vk::AccessFlags{},
			to_vk_access_flags_for_writing(aMemoryToBeMadeAvailable),
			aImage.current_layout(),
			aImage.target_layout(),
			0u, 0u, // <-- to be set later
			aImage.image_handle(),
			aImage.entire_subresource_range()
		);
		return *this;
	}

	sync& sync::make_image_available_for_reading_in_stages(const image_t& aImage, memory_stage aMemoryToBeMadeAvailable)
	{
		mBarrierParams.mImageMemoryBarriers.emplace_back(
			vk::AccessFlags{},
			to_vk_access_flags_for_reading(aMemoryToBeMadeAvailable),
			aImage.current_layout(),
			aImage.target_layout(),
			0u, 0u, // <-- to be set later
			aImage.image_handle(),
			aImage.entire_subresource_range()
		);
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

	sync& sync::set_pipeline_barrier_parameters_alteration_handler(std::function<void(pipeline_barrier_parameters&)> aHandler)
	{
		mAlterPipelineBarrierParametersHandler = std::move(aHandler);
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

	void sync::set_sync_stages_and_establish_barrier(command_buffer& aCommandBuffer, std::optional<pipeline_stage> aSourcePipelineStages, std::optional<memory_stage> aSourceMemoryStages)
	{
		// At this point, complete all the (prepared) synchronization settings in pipeline_barrier_parameters
		// Most importantly, set all the source stages:
		const auto sourcePipelineStages = to_vk_pipeline_stage_flags(aSourcePipelineStages);
		const auto sourceMemoryStages = to_vk_access_flags_for_writing(aSourceMemoryStages);
		mBarrierParams.mSrcStageMask = sourcePipelineStages;
		for (auto& mb : mBarrierParams.mMemoryBarriers) {
			mb.srcAccessMask = sourceMemoryStages;
		}
		for (auto& ib : mBarrierParams.mImageMemoryBarriers) {
			ib.srcAccessMask = sourceMemoryStages;
			if (queues_are_the_same()) {
				ib.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				ib.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			}
			else {
				ib.srcQueueFamilyIndex = queue_to_use().get().family_index();
				ib.dstQueueFamilyIndex = queue_to_transfer_to().get().family_index();	
			}
		}
		for (auto& bb : mBarrierParams.mBufferMemoryBarriers) { // TODO: Implement buffer memory barriers!
			bb.srcAccessMask = sourceMemoryStages;
			if (queues_are_the_same()) {
				bb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				bb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			}
			else {
				bb.srcQueueFamilyIndex = queue_to_use().get().family_index();
				bb.dstQueueFamilyIndex = queue_to_transfer_to().get().family_index();	
			}
		}

		// Allow the alteration of the barrier params, if a handler has been set
		if (mAlterPipelineBarrierParametersHandler) {
			mAlterPipelineBarrierParametersHandler(mBarrierParams);
		}

		// Now, for the "establish_barrier"-part:
		if (sync_type::via_barrier != get_sync_type()) {
			return; // nothing to do in this case
		}

		// Do it:	
		aCommandBuffer->handle().pipelineBarrier(
			mBarrierParams.mSrcStageMask, mBarrierParams.mDstStageMask,
			{}, // TODO: Dependency flags might become relevant with framebuffers and sub-passes (e.g. like the "by region" setting which can be a huge improvement on mobile)
			static_cast<uint32_t>(mBarrierParams.mMemoryBarriers.size()),		mBarrierParams.mMemoryBarriers.data(),
			static_cast<uint32_t>(mBarrierParams.mBufferMemoryBarriers.size()), mBarrierParams.mBufferMemoryBarriers.data(),
			static_cast<uint32_t>(mBarrierParams.mImageMemoryBarriers.size()),	mBarrierParams.mImageMemoryBarriers.data()
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
				if (mBarrierParams.mDstStageMask) {
					sema->set_semaphore_wait_stage(mBarrierParams.mDstStageMask);
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
