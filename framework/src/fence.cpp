#include <cg_base.hpp>

namespace cgb
{
	fence_t::~fence_t()
	{
		if (mCustomDeleter.has_value() && *mCustomDeleter) {
			// If there is a custom deleter => call it now
			(*mCustomDeleter)();
			mCustomDeleter.reset();
		}
		// Destroy the dependant instance before destroying myself
		// ^ This is ensured by the order of the members
		//   See: https://isocpp.org/wiki/faq/dtors#calling-member-dtors
	}

	fence_t& fence_t::set_designated_queue(device_queue& _Queue)
	{
		mQueue = &_Queue;
		return *this;
	}

	void fence_t::wait_until_signalled() const
	{
		cgb::context().logical_device().waitForFences(1u, handle_ptr(), VK_TRUE, UINT64_MAX);
	}

	void fence_t::reset()
	{
		cgb::context().logical_device().resetFences(1u, handle_ptr());
		if (mCustomDeleter.has_value() && *mCustomDeleter) {
			// If there is a custom deleter => call it now
			(*mCustomDeleter)();
			mCustomDeleter.reset();
		}
	}

	cgb::owning_resource<fence_t> fence_t::create(bool _CreateInSignaledState, context_specific_function<void(fence_t&)> _AlterConfigBeforeCreation)
	{
		fence_t result;
		result.mCreateInfo = vk::FenceCreateInfo()
			.setFlags(_CreateInSignaledState 
						? vk::FenceCreateFlagBits::eSignaled
						: vk::FenceCreateFlags() 
			);

		// Maybe alter the config?
		if (_AlterConfigBeforeCreation.mFunction) {
			_AlterConfigBeforeCreation.mFunction(result);
		}

		result.mFence = context().logical_device().createFenceUnique(result.mCreateInfo);
		return result;
	}
}
