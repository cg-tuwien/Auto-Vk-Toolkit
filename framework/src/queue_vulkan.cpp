#include <cg_base.hpp>

namespace cgb
{
	std::deque<device_queue> device_queue::sPreparedQueues;

	device_queue* device_queue::prepare(
		vk::QueueFlags pFlagsRequired,
		device_queue_selection_strategy pSelectionStrategy,
		std::optional<vk::SurfaceKHR> pSupportForSurface)
	{
		auto families = context().find_best_queue_family_for(pFlagsRequired, pSelectionStrategy, pSupportForSurface);
		if (families.size() == 0) {
			throw std::runtime_error("Couldn't find queue families satisfying the given criteria.");
		}

		// Default to the first ones, each
		uint32_t familyIndex = std::get<0>(families[0]);
		uint32_t queueIndex = 0;

		for (auto& family : families) {
			for (uint32_t qi = 0; qi < std::get<1>(family).queueCount; ++qi) {

				auto alreadyInUse = std::find_if(
					std::begin(sPreparedQueues), 
					std::end(sPreparedQueues), 
					[familyIndexInQuestion = std::get<0>(family), queueIndexInQuestion = qi](const auto& pq) {
					return pq.family_index() == familyIndexInQuestion
						&& pq.queue_index() == queueIndexInQuestion;
				});

				// Pay attention to different selection strategies:
				switch (pSelectionStrategy)
				{
				case cgb::device_queue_selection_strategy::prefer_separate_queues:
					if (sPreparedQueues.end() == alreadyInUse) {
						// didn't find combination, that's good
						familyIndex = std::get<0>(family);
						queueIndex = qi;
						goto found_indices;
					}
					break;
				case cgb::device_queue_selection_strategy::prefer_everything_on_single_queue:
					if (sPreparedQueues.end() != alreadyInUse) {
						// find combination, that's good in this case
						familyIndex = std::get<0>(family);
						queueIndex = 0; // => 0 ... i.e. everything on queue #0
						goto found_indices;
					}
					break;
				}
			}
		}

	found_indices:
		auto& prepd_queue = sPreparedQueues.emplace_back();
		prepd_queue.mQueueFamilyIndex = familyIndex;
		prepd_queue.mQueueIndex = queueIndex;
		prepd_queue.mPriority = 0.5f; // default priority of 0.5
		prepd_queue.mQueue = nullptr;
		return &prepd_queue;
	}

	device_queue device_queue::create(uint32_t pQueueFamilyIndex, uint32_t pQueueIndex)
	{
		device_queue result;
		result.mQueueFamilyIndex = pQueueFamilyIndex;
		result.mQueueIndex = pQueueIndex;
		result.mPriority = 0.5f; // default priority of 0.5f
		result.mQueue = context().logical_device().getQueue(result.mQueueFamilyIndex, result.mQueueIndex);
		return result;
	}

	device_queue device_queue::create(const device_queue& pPreparedQueue)
	{
		device_queue result;
		result.mQueueFamilyIndex = pPreparedQueue.family_index();
		result.mQueueIndex = pPreparedQueue.queue_index();
		result.mPriority = pPreparedQueue.mPriority; // default priority of 0.5f
		result.mQueue = context().logical_device().getQueue(result.mQueueFamilyIndex, result.mQueueIndex);
		return result;
	}

	command_pool& device_queue::pool() const 
	{ 
		return context().get_command_pool_for_queue(*this); 
	}

	semaphore device_queue::submit_and_handle_with_semaphore(command_buffer _CommandBuffer, std::vector<semaphore> _WaitSemaphores)
	{
		if (_CommandBuffer.state() != command_buffer_state::finished_recording) {
			throw std::runtime_error("Command buffer is still in recording-state, but it should be submitted. Have you forgotten to call `end_recording()`?");
		}
		
		// Create a semaphore which can, or rather, MUST be used to wait for the results
		auto signalWhenCompleteSemaphore = semaphore_t::create();
		signalWhenCompleteSemaphore->set_designated_queue(*this); //< Just store the info

		if (0 == _WaitSemaphores.size()) {
			// Optimized route for 0 _WaitSemaphores
			const auto submitInfo = vk::SubmitInfo()
				.setCommandBufferCount(1u)
				.setPCommandBuffers(_CommandBuffer.handle_addr())
				.setWaitSemaphoreCount(0u)
				.setPWaitSemaphores(nullptr)
				.setPWaitDstStageMask(nullptr)
				.setSignalSemaphoreCount(1u)
				.setPSignalSemaphores(signalWhenCompleteSemaphore->handle_addr());

			handle().submit({ submitInfo }, nullptr);
			_CommandBuffer.mState = command_buffer_state::submitted;

			signalWhenCompleteSemaphore->set_custom_deleter([
				ownedCommandBuffer{ std::move(_CommandBuffer) } // Take care of the command_buffer's lifetime.. OMG!
			](){});
		}
		else {
			// Also set the wait semaphores and take care of their lifetimes
			std::vector<vk::Semaphore> waitSemaphoreHandles;
			waitSemaphoreHandles.reserve(_WaitSemaphores.size());
			std::vector<vk::PipelineStageFlags> waitDstStageMasks;
			waitDstStageMasks.reserve(_WaitSemaphores.size());
			
			for (const auto& semaphoreDependency : _WaitSemaphores) {
				waitSemaphoreHandles.push_back(semaphoreDependency->handle());
				waitDstStageMasks.push_back(semaphoreDependency->semaphore_wait_stage());
			}
			
			const auto submitInfo = vk::SubmitInfo()
				.setCommandBufferCount(1u)
				.setPCommandBuffers(_CommandBuffer.handle_addr())
				.setWaitSemaphoreCount(static_cast<uint32_t>(waitSemaphoreHandles.size()))
				.setPWaitSemaphores(waitSemaphoreHandles.data())
				.setPWaitDstStageMask(waitDstStageMasks.data())
				.setSignalSemaphoreCount(1u)
				.setPSignalSemaphores(signalWhenCompleteSemaphore->handle_addr());

			handle().submit({ submitInfo }, nullptr);
			_CommandBuffer.mState = command_buffer_state::submitted;

			signalWhenCompleteSemaphore->set_custom_deleter([
				ownedWaitSemaphores{ std::move(_WaitSemaphores) },
				ownedCommandBuffer{ std::move(_CommandBuffer) } // Take care of the command_buffer's lifetime.. OMG!
			](){});	
		}
		
		return signalWhenCompleteSemaphore;
	}
}
