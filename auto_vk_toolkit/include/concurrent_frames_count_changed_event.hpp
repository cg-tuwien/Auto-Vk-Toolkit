#pragma once
#include "window.hpp"
#include "event.hpp"

namespace avk
{
	/** This event occurs when the number of concurrent frames changes.
	 */
	class concurrent_frames_count_changed_event : public event
	{
	public:
		concurrent_frames_count_changed_event(window* aWindow) : mWindow{ aWindow }, framesInFlight{ aWindow->get_config_number_of_concurrent_frames() } {}
		concurrent_frames_count_changed_event(concurrent_frames_count_changed_event&&) noexcept = default;
		concurrent_frames_count_changed_event(const concurrent_frames_count_changed_event&) = default;
		concurrent_frames_count_changed_event& operator=(concurrent_frames_count_changed_event&&) noexcept = default;
		concurrent_frames_count_changed_event& operator=(const concurrent_frames_count_changed_event&) = default;
		~concurrent_frames_count_changed_event() = default;

		bool update(event_data& aData) override
		{
			if (auto currentFramesInFlights = mWindow->get_config_number_of_concurrent_frames(); currentFramesInFlights != framesInFlight) {
				framesInFlight = currentFramesInFlights;
				return true;
			}

			return false;
		}

		auto* get_window() { return mWindow; }
		const auto* get_window() const { return mWindow; }

	private:
		window::frame_id_t framesInFlight;
		window* mWindow;
	};

	static bool operator==(const concurrent_frames_count_changed_event& left, const concurrent_frames_count_changed_event& right)
	{
		return left.get_window() == right.get_window();
	}

	static bool operator!=(const concurrent_frames_count_changed_event& left, const concurrent_frames_count_changed_event& right)
	{
		return !(left == right);
	}
}

namespace std // Inject hash for `concurrent_frames_count_changed_event` into std::
{
	template<> struct hash<avk::concurrent_frames_count_changed_event>
	{
		std::size_t operator()(avk::concurrent_frames_count_changed_event const& o) const noexcept
		{
			std::size_t h = 0;
			avk::hash_combine(h, o.get_window());
			return h;
		}
	};
}
