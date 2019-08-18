#pragma once

namespace cgb
{

	/** A synchronization object which allows GPU->GPU synchronization */
	class semaphore_t
	{
	public:
		semaphore_t() = default;
		semaphore_t(const semaphore_t&) = delete;
		semaphore_t(semaphore_t&&) = default;
		semaphore_t& operator=(const semaphore_t&) = delete;
		semaphore_t& operator=(semaphore_t&&) = default;
		virtual ~semaphore_t();

		static cgb::owning_resource<semaphore_t> create(context_specific_function<void(semaphore_t&)> _AlterConfigBeforeCreation = {});

	};

	// Typedef for a variable representing an owner of a semaphore
	using semaphore = owning_resource<semaphore_t>;
}
