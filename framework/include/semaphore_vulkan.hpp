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
		semaphore_t(const semaphore_t&) = delete;
		semaphore_t(semaphore_t&&) = default;
		semaphore_t& operator=(const semaphore_t&) = delete;
		semaphore_t& operator=(semaphore_t&&) = default;
		virtual ~semaphore_t();

		/**	Set a queue where this semaphore is designated to be submitted to.
		 *	This is only used for keeping the information of the queue. 
		 *	There is no impact on any internal functionality whether or not a designated queue has been set.
		 */
		semaphore_t& set_designated_queue(device_queue& _Queue);

		/**	Set the stage where the semaphore wait shall occur.
		 *	The next command, which waits on this semaphore, shall use this info
		 *	to wait for the optimal stage. This value is intended to be used for
		 *	`VkSubmitInfo::pWaitStageMask`.
		 *	Use `semaphore_wait_stage()` to query the set value.
		 *	The default value is `vk::PipelineStageFlagBits::eAllCommands`
		 */
		semaphore_t& set_semaphore_wait_stage(vk::PipelineStageFlags _Stage);

		/** Hand over control of another semaphore which this semaphore implicitly depends on.
		 *	I.e. the dependent semaphore shall complete before this semaphore.
		// */
		//semaphore_t& set_dependent_semaphore(owning_resource<semaphore_t> _DependentSemaphore)
		//{
		//	mDependentSemaphore = std::make_unique<owning_resource<semaphore_t>>( std::move(_DependentSemaphore) );
		//	return *this;
		//}

		template <typename F>
		semaphore_t& set_custom_deleter(F&& _Deleter) 
		{
			if (mCustomDeleter) {
				// There is already a custom deleter! Make sure that this stays alive as well.
				mCustomDeleter = [
					existingDeleter = std::move(mCustomDeleter),
					additionalDeleter = std::forward<F>(_Deleter)
				]() {};
			}
			else {
				mCustomDeleter = std::forward<F>(_Deleter);
			}
			return *this;
		}

		const auto& create_info() const { return mCreateInfo; }
		const auto& handle() const { return mSemaphore.get(); }
		const auto* handle_addr() const { return &mSemaphore.get(); }
		auto has_designated_queue() const { return nullptr != mQueue; }
		auto* designated_queue() const { return mQueue; }
		
		/**	The stage where the semaphore occurs and which the next command shall wait on,
		 *	or `vk::PipelineStageFlagBits::eAllCommands`.
		 */
		auto semaphore_wait_stage() const { return mSemaphoreWaitStageForNextCommand; }
		const auto* semaphore_wait_stage_addr() const { return &mSemaphoreWaitStageForNextCommand; }

		static cgb::owning_resource<semaphore_t> create(context_specific_function<void(semaphore_t&)> _AlterConfigBeforeCreation = {});

		/**	Use this method to wait until the operation, which the semaphore represents, has completed
		 *	by either waiting for idle on the queue or by waiting for idle on the device.
		 */
		void wait_idle() const;

	private:
		// The semaphore config struct:
		vk::SemaphoreCreateInfo mCreateInfo;
		// The semaphore handle:
		vk::UniqueSemaphore mSemaphore;
		// A queue, which this semaphore is assigned to, or nullptr
		device_queue* mQueue;
		// Info for the next command, at which stage the semaphore wait should occur.
		vk::PipelineStageFlags mSemaphoreWaitStageForNextCommand;

		// --- Some advanced features of a semaphore object ---

		/** An optional dependent semaphore. This means: The dependent
		*	semaphore can be assumed to be finished when this semaphore
		*	has finished.
		*	The point here is that some internal function might wait on it,
		*	that shall be somewhat opaque to the user in some cases.
		*	The dependent semaphore child object ensures that the semaphore
		*	does not get destructed prematurely.
		*/
		//std::unique_ptr<owning_resource<semaphore_t>> mDependentSemaphore;

		/** A custom deleter function called upon destruction of this semaphore */
		std::optional<cgb::unique_function<void()>> mCustomDeleter;
	};

	// Typedef for a variable representing an owner of a semaphore
	using semaphore = owning_resource<semaphore_t>;

	/**	Handle a semaphore using a semaphore handler or via waitIdle
	 */
	void handle_semaphore(semaphore _SemaphoreToHandle, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler = {});
}
