#include <cg_base.hpp>

namespace cgb
{
	semaphore_t::semaphore_t()
		: mCreateInfo{}
		, mSemaphore{}
		, mQueue{ nullptr }
		, mSemaphoreWaitStageForNextCommand{ vk::PipelineStageFlagBits::eAllCommands }
		, mCustomDeleter{}
	{
	}

	semaphore_t::~semaphore_t()
	{
		if (mCustomDeleter.has_value() && *mCustomDeleter) {
			// If there is a custom deleter => call it now
			(*mCustomDeleter)();
			mCustomDeleter.reset();
		}
		// Destroy the dependant semaphore before destroying "my actual semaphore"
		// ^ This is ensured by the order of the members
		//   See: https://isocpp.org/wiki/faq/dtors#calling-member-dtors
	}

	semaphore_t& semaphore_t::set_designated_queue(device_queue& _Queue)
	{
		mQueue = &_Queue;
		return *this;
	}

	semaphore_t& semaphore_t::set_semaphore_wait_stage(vk::PipelineStageFlags _Stage)
	{
		mSemaphoreWaitStageForNextCommand = _Stage;
		return *this;
	}

	cgb::owning_resource<semaphore_t> semaphore_t::create(context_specific_function<void(semaphore_t&)> _AlterConfigBeforeCreation)
	{ 
		semaphore_t result;
		result.mCreateInfo = vk::SemaphoreCreateInfo{};

		// Maybe alter the config?
		if (_AlterConfigBeforeCreation.mFunction) {
			_AlterConfigBeforeCreation.mFunction(result);
		}

		result.mSemaphore = context().logical_device().createSemaphoreUnique(result.mCreateInfo);
		return result;
	}

	void semaphore_t::wait_idle() const
	{
		if (has_designated_queue()) {
			LOG_WARNING("semaphore_t::wait_idle() has been called - most likely as a consequence of a missing semaphore handler. Will block the queue via waitIdle until the operation has completed.");
			designated_queue()->handle().waitIdle();
		}
		else {
			LOG_WARNING("semaphore_t::wait_idle() has been called - most likely as a consequence of a missing semaphore handler. Will block the device via waitIdle until the operation has completed.");
			cgb::context().logical_device().waitIdle();
		}
	}

	void handle_semaphore(semaphore _SemaphoreToHandle, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler)
	{
		if (_SemaphoreHandler) { // Did the user provide a handler?
			_SemaphoreHandler(std::move(_SemaphoreToHandle)); // Transfer ownership and be done with it
		}
		else {
			_SemaphoreToHandle->wait_idle();
		}
	}
}
