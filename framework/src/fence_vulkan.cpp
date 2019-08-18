namespace cgb
{
	cgb::owning_resource<fence_t> fence_t::create(bool _CreateInSignaledState, context_specific_function<void(fence_t&)> _AlterConfigBeforeCreation)
	{
		fence_t result;
		result.mCreateInfo = vk::FenceCreateInfo()
			.setFlags(_CreateInSignaledState ? vk::FenceCreateFlags() : vk::FenceCreateFlagBits::eSignaled);

		// Maybe alter the config?
		if (_AlterConfigBeforeCreation.mFunction) {
			_AlterConfigBeforeCreation.mFunction(result);
		}

		result.mFence = context().logical_device().createFenceUnique(result.mCreateInfo);
		return result;
	}
}
