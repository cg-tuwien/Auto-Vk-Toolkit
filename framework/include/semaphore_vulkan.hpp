#pragma once

namespace cgb
{
	// Forward declaration:
	class device_queue;

	/** A synchronization object which allows GPU->GPU synchronization */
	class semaphore_t
	{
	public:
		semaphore_t();
		semaphore_t(semaphore_t&&) noexcept = default;
		semaphore_t(const semaphore_t&) = delete;
		semaphore_t& operator=(semaphore_t&&) noexcept = default;
		semaphore_t& operator=(const semaphore_t&) = delete;
		~semaphore_t();

		/**	Stage where to wait for this semaphore, i.e. stage which the following operation has to wait for.
		 *	The default value is `vk::PipelineStageFlagBits::eAllCommands`
		 */
		semaphore_t& set_semaphore_wait_stage(vk::PipelineStageFlags _Stage);

		/** Set a custom deleter function.
		 *	This is often used for resource cleanup, e.g. a buffer which can be deleted when this semaphore is destroyed.
		 */
		template <typename F>
		semaphore_t& set_custom_deleter(F&& aDeleter) noexcept
		{
			if (mCustomDeleter.has_value()) {
				// There is already a custom deleter! Make sure that this stays alive as well.
				mCustomDeleter = [
					existingDeleter = std::move(mCustomDeleter.value()),
					additionalDeleter = std::forward<F>(aDeleter)
				]() {
					additionalDeleter();
					existingDeleter();
				};
			}
			else {
				mCustomDeleter = std::forward<F>(aDeleter);
			}
			return *this;
		}

		const auto& create_info() const { return mCreateInfo; }
		const auto& handle() const { return mSemaphore.get(); }
		const auto* handle_addr() const { return &mSemaphore.get(); }
		
		/**	Gets the stage which the subsequent command shall wait for.
		 *	If no stage has been set, the default value is `vk::PipelineStageFlagBits::eAllCommands`.
		 */
		auto semaphore_wait_stage() const { return mSemaphoreWaitStageForNextCommand; }
		/** Gets the address of the `semaphore_wait_stage`. */
		const auto* semaphore_wait_stage_addr() const { return &mSemaphoreWaitStageForNextCommand; }

		static cgb::owning_resource<semaphore_t> create(context_specific_function<void(semaphore_t&)> _AlterConfigBeforeCreation = {});

	private:
		// The semaphore config struct:
		vk::SemaphoreCreateInfo mCreateInfo;
		// The semaphore handle:
		vk::UniqueSemaphore mSemaphore;
		// Info for the next command, at which stage the semaphore wait should occur.
		vk::PipelineStageFlags mSemaphoreWaitStageForNextCommand;

		/** A custom deleter function called upon destruction of this semaphore */
		std::optional<cgb::unique_function<void()>> mCustomDeleter;
	};

	// Typedef for a variable representing an owner of a semaphore
	using semaphore = owning_resource<semaphore_t>;
}
