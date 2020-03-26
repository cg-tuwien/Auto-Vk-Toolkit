#pragma once

namespace cgb
{
	// The context_tracker gets included before the context. Therefore, we need
	// to forward-declare the context methods
#if defined(USE_OPENGL_CONTEXT)
	class opengl;
	inline opengl& context();
#elif defined(USE_VULKAN_CONTEXT)
	class vulkan;
	inline vulkan& context();
#endif

	template <typename T>
	class context_tracker
	{
	public:
		context_tracker() : mTrackee{ nullptr }
		{ }

		void setTrackee(T* pTrackee) noexcept
		{
			if (nullptr == mTrackee) {
				mTrackee = pTrackee;
				context().track_creation(mTrackee);
			}
			else {
				LOG_ERROR(fmt::format("Usage error: Do not set the trackee more than once! Type: '{}'", typeid(T).name()));
				context().track_destruction(mTrackee);
				mTrackee = pTrackee;
				context().track_creation(mTrackee);
			}
		}

		void setTrackee(T& pTrackee) noexcept
		{
			setTrackee(&pTrackee);
		}

		context_tracker(T* pTrackee) noexcept : mTrackee{ pTrackee }
		{
			context().track_creation(mTrackee);
		}

		context_tracker(const context_tracker<T>& other) noexcept
		{
			context().track_copy(mTrackee, other.mTrackee);
		}

		context_tracker(context_tracker<T>&& other) noexcept
		{
			context().track_move(mTrackee, other.mTrackee);
		}

		context_tracker<T>& operator=(const context_tracker<T>& other) noexcept
		{
			context().track_copy(mTrackee, other.mTrackee);
			return *this;
		}

		context_tracker<T>& operator=(context_tracker<T>&& other) noexcept
		{
			context().track_move(mTrackee, other.mTrackee);
			return *this;
		}

		~context_tracker()
		{
			context().track_destruction(mTrackee);
		}

	private:
		T* mTrackee;
	};
}
