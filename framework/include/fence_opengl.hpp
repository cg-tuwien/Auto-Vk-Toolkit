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

		static cgb::owning_resource<fence_t> create(bool _CreateInSignaledState = false, context_specific_function<void(fence_t&)> _AlterConfigBeforeCreation = {});

	};

	using fence = cgb::owning_resource<fence_t>;
}
