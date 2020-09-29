#pragma once
#include <gvk.hpp>
#include <FileWatcher/FileWatcher.h>

namespace gvk
{
	class updater;
	using event_t = std::variant<std::shared_ptr<event>, files_changed_event, swapchain_changed_event, swapchain_resized_event>;
	using updatee_t = std::variant<avk::graphics_pipeline, avk::compute_pipeline, avk::ray_tracing_pipeline>;

	struct update_and_determine_fired
	{
		void operator()(std::shared_ptr<event>& e)	{ mFired = e->update(mEventData); }
		void operator()(files_changed_event& e)		{ mFired = e.update(mEventData); }
		void operator()(swapchain_changed_event& e)	{ mFired = e.update(mEventData); }
		void operator()(swapchain_resized_event& e)	{ mFired = e.update(mEventData); }
		event_data mEventData;
		bool mFired = false;
	};

	struct recreate_updatee
	{
		void operator()(avk::graphics_pipeline& u);
		void operator()(avk::compute_pipeline& u);
		void operator()(avk::ray_tracing_pipeline& u);
		event_data& mEventData;
		std::optional<updatee_t> mUpdateeToCleanUp;
	};

	struct updater_config_proxy
	{
		template <typename... Updatees>
		void update(Updatees... updatees);
		
		updater* mUpdater;
		uint64_t mEventIndicesBitset;
		window::frame_id_t mTtl;
	};

	// An updater that can handle different types of updates
	class updater : public invokee
	{
	public:
		// Make sure, an updater runs first in render()
		int execution_order() const override { return std::numeric_limits<int>::min(); }
		
		void render() override;

		template <typename E>
		uint8_t get_event_index_and_possibly_add_event(E e)
		{
			auto it = std::find_if(std::begin(mEvents), std::end(mEvents), [&e](auto& x){
				if (!std::holds_alternative<E>(x)) {
					return false;
				}
				return std::get<E>(x) == e;
			});
			if (std::end(mEvents) != it) {
				return static_cast<uint8_t>(std::distance(std::begin(mEvents), it));
			}
			if (mEvents.size() == cMaxEvents) {
				throw gvk::runtime_error(fmt::format("There can not be more than {} events handled by one updater instance. Sorry.", cMaxEvents));
			}
			mEvents.emplace_back(std::move(e));
			return static_cast<uint8_t>(mEvents.size() - 1);
		}

		window::frame_id_t get_ttl(std::shared_ptr<event>& e)	{ return 0; }
		window::frame_id_t get_ttl(files_changed_event& e)		{ return 0; }
		window::frame_id_t get_ttl(swapchain_changed_event& e)	{ return e.get_window()->number_of_frames_in_flight(); }
		window::frame_id_t get_ttl(swapchain_resized_event& e)	{ return e.get_window()->number_of_frames_in_flight(); }

		template <typename... Events>
		updater_config_proxy on(Events... events)
		{
			// determine ttl
			window::frame_id_t ttl = 0;
			((ttl = std::max(ttl, get_ttl(events))), ...);
			if (0 == ttl) {
				gvk::context().execute_for_each_window([&ttl](window* w){
					ttl = std::max(ttl, w->number_of_frames_in_flight());
				});
			}
			
			updater_config_proxy result{this, 0u, ttl};
			((result.mEventIndicesBitset |= (uint64_t{1} << get_event_index_and_possibly_add_event(std::move(events)))), ...);
			return result;
		}

		void add_updatee(uint64_t aEventsBitset, updatee_t aUpdatee, window::frame_id_t aTtl);

	private:
		const size_t cMaxEvents = 64;
		window::frame_id_t mCurrentUpdaterFrame = 0;

		// List of events. Must not be something that moves elements around once initialized.
		// (For some event-classes, it would work, but for some it does not, like for files_changed_event, which installs a callback to itself.)
		std::deque<event_t> mEvents;

		// List of resources to be updated + additional data. Contents of the tuple as follows:
		//          [0]: event-mapping ... Which events cause this updatee to be updated
		//          [1]: the updatee ..... The thing to be updated (i.e. have its guts swapped under the hood)
		//          [2]: time to live .... Number of "frames" an updatee is kept alive before it is destroyed,
		//			                       where "frames" actually means: updater's render() invocations.
		std::vector<std::tuple<uint64_t, updatee_t, window::frame_id_t>> mUpdatees;

		// List will be cleaned from the front. Resources will be cleaned if they have surpassed the frame-id
		// stored in the tuple's first element. The resource to be deleted is stored in the tuple's second element.
		std::deque<std::tuple<window::frame_id_t, updatee_t>> mUpdateesToCleanUp;
	};

	template <typename... Updatees>
	void updater_config_proxy::update(Updatees... updatees)
	{
		(mUpdater->add_updatee(mEventIndicesBitset, updatees, mTtl), ...);
	}
}
