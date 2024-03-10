#pragma once

#include <FileWatcher/FileWatcher.h>

#include "concurrent_frames_count_changed_event.hpp"
#include "destroying_events.hpp"
#include "event.hpp"
#include "files_changed_event.hpp"
#include "swapchain_resized_event.hpp"
#include "swapchain_changed_event.hpp"
#include "swapchain_format_changed_event.hpp"
#include "swapchain_additional_attachments_changed_event.hpp"

#include "context_vulkan.hpp"


namespace avk
{
	class updater;
	using event_t = std::variant<
		std::shared_ptr<event>,
		files_changed_event,
		swapchain_changed_event,
		swapchain_resized_event,
		swapchain_format_changed_event,
		concurrent_frames_count_changed_event,
		swapchain_additional_attachments_changed_event,
		destroying_graphics_pipeline_event,
		destroying_compute_pipeline_event,
		destroying_ray_tracing_pipeline_event,
		destroying_image_event,
		destroying_image_view_event
	>;
	using event_handler_t = std::variant<
		std::function<void()>,
		std::function<void(const avk::graphics_pipeline&)>,
		std::function<void(const avk::compute_pipeline&)>,
		std::function<void(const avk::ray_tracing_pipeline&)>,
		std::function<void(const avk::image&)>,
		std::function<void(const avk::image_view&)>
	>;
	using resource_updatee_t = std::variant<avk::graphics_pipeline, avk::compute_pipeline, avk::ray_tracing_pipeline, avk::image, avk::image_view>;
	using updatee_t = std::variant<avk::graphics_pipeline, avk::compute_pipeline, avk::ray_tracing_pipeline, avk::image, avk::image_view, event_handler_t>;

	/** Concept that allows variadic templates used only on resource_updatee_t type
	 */
	template <typename T>
	concept is_updatee = requires (T& aT, resource_updatee_t& aVar)
	{
		aVar = aT;
	};

	/** Concept that allows variadic templates used only on event_handler_t type
	 */
	template <typename T>
	concept is_event_handler = requires (T& aT, event_handler_t& aVar)
	{
		aVar = aT;
	};

	struct update_operations_data
	{
		void operator()(avk::graphics_pipeline& u);
		void operator()(avk::compute_pipeline& u);
		void operator()(avk::ray_tracing_pipeline& u);
		void operator()(avk::image& u);
		void operator()(avk::image_view& u);
		void operator()(event_handler_t& u);
		event_data& mEventData;
		std::optional<updatee_t> mUpdateeToCleanUp;
	};

	class updater_config_proxy
	{
		friend class updater;

	public:
		updater_config_proxy(updater* aUpdater, uint64_t aEventIndicesBitset, window::frame_id_t aTtl)
			: mUpdater{ aUpdater }
			, mEventIndicesBitset{ aEventIndicesBitset }
			, mTtl{ aTtl }
		{ }

		/**	Specify resources that should be updated by the updater on the occurence of one/multiple event/s.
		 *	Supported values are:
		 *	 - avk::graphics_pipeline
		 *	 - avk::compute_pipeline
		 *	 - avk::ray_tracing_pipeline
		 *	 - avk::image
		 *	 - avk::image_view
		 */
		template <is_updatee... Updatees>
		updater_config_proxy& update(Updatees... updatees);

		/**	Specify event handlers that should be invoked by the updater on the occurence of one/multiple event/s.
		 *	Supported function signatures are:
		 *	 - void() .................................. Can be used generally.
		 *	 - void(const avk::graphics_pipeline&) ..... To be used in conjunction with on(destroying_graphics_pipeline_event()). The pipeline to be destroyed is passed as the first parameter.
		 *	 - void(const avk::compute_pipeline&) ...... To be used in conjunction with on(destroying_compute_pipeline_event()). The pipeline to be destroyed is passed as the first parameter.
		 *	 - void(const avk::ray_tracing_pipeline&) .. To be used in conjunction with on(destroying_ray_tracing_pipeline_event()). The pipeline to be destroyed is passed as the first parameter.
		 *	 - void(const avk::image&) ................. To be used in conjunction with on(destroying_image_event()). The image to be destroyed is passed as the first parameter.
		 *	 - void(const avk::image_view&) ............ To be used in conjunction with on(destroying_image_view_event()). The image view to be destroyed is passed as the first parameter.
		 */
		template <is_event_handler... Handlers>
		updater_config_proxy& invoke(Handlers... handlers);

		template <typename... Events>
		updater_config_proxy then_on(Events... events);


	private:
		updater* mUpdater;
		uint64_t mEventIndicesBitset;
		window::frame_id_t mTtl;
	};

	// An updater that can handle different types of updates
	class updater
	{
	public:

		friend class updater_config_proxy;
		/** @brief this function applies the updates as instructed.
		* As long as this updater is active as a part of invokee, for instance, then
		* this function is automatically called on the appropriate moment (that is
		* before the render call of that invokee).
		*/
		void apply();

		/** this function executes any static calls that are meant to be invoked once per frame
		 */
		static void prepare_for_current_frame();

		template <typename E>
		uint8_t get_event_index_and_possibly_add_event(E e, const size_t aBeginOffset = 0)
		{
			auto it = std::find_if(std::begin(mEvents) + aBeginOffset, std::end(mEvents), [&e](auto& x){
				if (!std::holds_alternative<E>(x)) {
					return false;
				}
				return std::get<E>(x) == e;
			});
			if (std::end(mEvents) != it) {
				return static_cast<uint8_t>(std::distance(std::begin(mEvents), it));
			}
			if (mEvents.size() == cMaxEvents) {
				throw avk::runtime_error(std::format("There can not be more than {} events handled by one updater instance. Sorry.", cMaxEvents));
			}
			mEvents.emplace_back(std::move(e));
			return static_cast<uint8_t>(mEvents.size() - 1);
		}

		window::frame_id_t get_ttl(std::shared_ptr<event>& e)                            { return 0; }
		window::frame_id_t get_ttl(files_changed_event& e)                               { return 0; }
		window::frame_id_t get_ttl(swapchain_changed_event& e)                           { return e.get_window()->number_of_frames_in_flight(); }
		window::frame_id_t get_ttl(swapchain_resized_event& e)                           { return e.get_window()->number_of_frames_in_flight(); }
		window::frame_id_t get_ttl(swapchain_format_changed_event& e)                    { return e.get_window()->number_of_frames_in_flight(); }
		window::frame_id_t get_ttl(concurrent_frames_count_changed_event& e)             { return 0; }
		window::frame_id_t get_ttl(swapchain_additional_attachments_changed_event& e)    { return 0; }
		window::frame_id_t get_ttl(destroying_graphics_pipeline_event& e)                { return 0; }
		window::frame_id_t get_ttl(destroying_compute_pipeline_event& e)                 { return 0; }
		window::frame_id_t get_ttl(destroying_ray_tracing_pipeline_event& e)             { return 0; }
		window::frame_id_t get_ttl(destroying_image_event& e)                            { return 0; }
		window::frame_id_t get_ttl(destroying_image_view_event& e)                       { return 0; }

		/**	Specify one or multiple events which shall trigger an action.
		 *  Possible events are the following:
		 *   - files_changed_event ............................ Triggered after files have changed on disk.                          Example: `avk::shader_files_changed_event(mPipeline)`
		 *   - swapchain_changed_event ........................ Triggered after the swapchain has changed.                           Example: `avk::swapchain_changed_event(context().main_window())`
		 *   - swapchain_resized_event ........................ Triggered after the swapchain has been resized.                      Example: `avk::swapchain_resized_event(context().main_window())`
		 *   - swapchain_format_changed_event ................. Triggered after the swapchain format has changed.                    Example: `avk::swapchain_format_changed_event(context().main_window())`
		 *   - concurrent_frames_count_changed_event........... Triggered after window #framesInFlight has changed.                  Example: `avk::concurrent_frames_count_changed_event(context().main_window())`
		 *   - swapchain_additional_attachments_changed_event.. Triggered after window additional attachments have changed.          Example: `avk::swapchain_additional_attachments_changed_event(context().main_window())`
		 *   - destroying_graphics_pipeline_event ............. Triggered before an old, outdated graphics pipeline is destroyed.    Example: `avk::destroying_graphics_pipeline_event()`
		 *   - destroying_compute_pipeline_event .............. Triggered before an old, outdated compute pipeline is destroyed.     Example: `avk::destroying_compute_pipeline_event()`
		 *   - destroying_ray_tracing_pipeline_event .......... Triggered before an old, outdated ray tracing pipeline is destroyed. Example: `avk::destroying_ray_tracing_pipeline_event()`
		 *   - destroying_image_event ......................... Triggered before an old, outdated image is destroyed.                Example: `avk::destroying_image_event()`
		 *   - destroying_image_view_event .................... Triggered before an old, outdated image view is destroyed.           Example: `avk::destroying_image_view_event()`
		 *
		 */
		template <typename... Events>
		updater_config_proxy on(Events... events)
		{
			// determine ttl
			window::frame_id_t ttl = 0;
			((ttl = std::max(ttl, get_ttl(events))), ...);
			if (0 == ttl) {
				context().execute_for_each_window([&ttl](window* w){
					ttl = std::max(ttl, w->number_of_frames_in_flight());
				});
			}

			updater_config_proxy result{this, 0u, ttl};
			((result.mEventIndicesBitset |= (uint64_t{1} << get_event_index_and_possibly_add_event(std::move(events)))), ...);
			return result;
		}

		void add_updatee(uint64_t aEventsBitset, updatee_t aUpdatee, window::frame_id_t aTtl);

	private:
		static constexpr size_t cMaxEvents = 64;
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

	/**
	* adds objects of type resource_updatee_t to the update list,
	* to be updated when the associated event is fired.
	*
	* @param aUpdatees		list of resource_updatee_t objects to be added.
	*/
	template <is_updatee... Updatees>
	updater_config_proxy& updater_config_proxy::update(Updatees... aUpdatees)
	{
		(mUpdater->add_updatee(mEventIndicesBitset, aUpdatees, mTtl), ...);
		return *this;
	};

	/**
	* adds event handlers to the update list, to be called when the associated event
	* is fired.
	*
	* @param aHandlers		list of handlers to be added.
	*/
	template <is_event_handler... Handlers>
	updater_config_proxy& updater_config_proxy::invoke(Handlers... aHandlers)
	{
		(mUpdater->add_updatee(mEventIndicesBitset, aHandlers, mTtl), ...);
		return *this;
	}

	/** adds a list of events to the chain which must be evaluated
	*	only after the previous events on the chain have invoked their callbacks
	*
	*	@param events		list of events
	*/
	template <typename... Events>
	updater_config_proxy updater_config_proxy::then_on(Events... events)
	{
		// determine ttl
		window::frame_id_t ttl = 0;
		((ttl = std::max(ttl, mUpdater->get_ttl(events))), ...);
		if (0 == ttl) {
			context().execute_for_each_window([&ttl](window* w) {
				ttl = std::max(ttl, w->number_of_frames_in_flight());
				});
		}

		// find the correct index offset for placing events on the evaluation list
		auto indicesBitset = mEventIndicesBitset; // cannot be zero, there is at least one event
		int offset = -1;
		while (indicesBitset != 0) {
			indicesBitset = indicesBitset >> 1;
			offset++;
		}

		updater_config_proxy result{ mUpdater, 0u, ttl };
		((result.mEventIndicesBitset |= (uint64_t{ 1 } << mUpdater->get_event_index_and_possibly_add_event(std::move(events), offset))), ...);
		return result;
	}
}
