#pragma once

namespace cgb
{
	/** A synchronization object which allows GPU->CPU synchronization */
	class fence_t
	{
	public:
		fence_t() = default;
		fence_t(const fence_t&) = delete;
		fence_t(fence_t&&) = default;
		fence_t& operator=(const fence_t&) = delete;
		fence_t& operator=(fence_t&&) = default;
		~fence_t() = default;

		const auto& create_info() const { return mCreateInfo; }
		const auto& handle() const { return mFence.get(); }
		const auto* handle_addr() const { return &mFence.get(); }

		static cgb::owning_resource<fence_t> create(bool _CreateInSignaledState = false, context_specific_function<void(fence_t&)> _AlterConfigBeforeCreation = {});

	private:
		vk::FenceCreateInfo mCreateInfo;
		vk::UniqueFence mFence;
	};

	using fence = cgb::owning_resource<fence_t>;
}
