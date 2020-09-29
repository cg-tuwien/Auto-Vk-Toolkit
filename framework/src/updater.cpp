#include <gvk.hpp>

namespace gvk
{
	void recreate_updatee::operator()(avk::graphics_pipeline& u)
	{
		auto newPipeline = gvk::context().create_graphics_pipeline_from_template(u, [&ed = mEventData](avk::graphics_pipeline_t& aPreparedPipeline){
			for (auto& vp : aPreparedPipeline.viewports()) {
				auto size = ed.get_extent_for_old_extent(vp.width, vp.height);
				vp.width = std::get<0>(size);
				vp.height = std::get<1>(size);
			}
			for (auto& sc : aPreparedPipeline.scissors()) {
				sc.extent = ed.get_extent_for_old_extent(sc.extent);
			}
		});
		newPipeline.enable_shared_ownership(); // Must be, otherwise updater can't handle it.
		std::swap(*newPipeline, *u);
		mUpdateeToCleanUp = std::move(newPipeline); // new == old by now
	}
	
	void recreate_updatee::operator()(avk::compute_pipeline& u)
	{
		auto newPipeline = gvk::context().create_compute_pipeline_from_template(u, [&ed = mEventData](avk::compute_pipeline_t& aPreparedPipeline){
			// TODO: Something to alter here?
		});
		newPipeline.enable_shared_ownership(); // Must be, otherwise updater can't handle it.
		std::swap(*newPipeline, *u);
		mUpdateeToCleanUp = std::move(newPipeline); // new == old by now
	}
	
	void recreate_updatee::operator()(avk::ray_tracing_pipeline& u)
	{
		auto newPipeline = gvk::context().create_ray_tracing_pipeline_from_template(u, [&ed = mEventData](avk::ray_tracing_pipeline_t& aPreparedPipeline){
			// TODO: Something to alter here?
		});
		newPipeline.enable_shared_ownership(); // Must be, otherwise updater can't handle it.
		std::swap(*newPipeline, *u);
		mUpdateeToCleanUp = std::move(newPipeline); // new == old by now
	}

	void updater::render()
	{
		// See if we have any resources to clean up:
		if (!mUpdateesToCleanUp.empty()) {
			// If there are different TTL values, it might happen that some resources are not cleaned at the earliest time, but a bit later.
			auto it = std::upper_bound(
				std::begin(mUpdateesToCleanUp), std::end(mUpdateesToCleanUp),
				mCurrentUpdaterFrame, [](window::frame_id_t fid, const auto& tpl){
					return fid < std::get<window::frame_id_t>(tpl); 
				}
			);
			mUpdateesToCleanUp.erase(std::begin(mUpdateesToCleanUp), it);
		}

		// First of all, perform a static update due to strange FileWatcher behavior :-/
		files_changed_event::update();
		// Then perform the individual updates:
		// 
		// See which events have fired:
		uint64_t eventsFired = 0;
		assert(cMaxEvents >= mEvents.size());
		update_and_determine_fired determinator{{}, false};
		const auto n = std::min(cMaxEvents, mEvents.size());
		for (size_t i = 0; i < n; ++i) {
			determinator.mFired = false;
			std::visit(determinator, mEvents[i]);
			if (determinator.mFired) {
				eventsFired |= (uint64_t{1} << i);
			}
		}

		// Update all who had at least one of their relevant events fire:
		for (auto& tpl : mUpdatees) {
			bool needsUpdate = (std::get<uint64_t>(tpl) & eventsFired) != 0;
			if (needsUpdate) {
				recreate_updatee recreator{determinator.mEventData, {}};
				std::visit(recreator, std::get<updatee_t>(tpl));
				if (recreator.mUpdateeToCleanUp.has_value()) {
					mUpdateesToCleanUp.emplace_back(mCurrentUpdaterFrame + std::get<window::frame_id_t>(tpl), std::move(recreator.mUpdateeToCleanUp.value()));
				}
			}
		}

		++mCurrentUpdaterFrame;
	}
	
	void updater::add_updatee(uint64_t aEventsBitset, updatee_t aUpdatee, window::frame_id_t aTtl)
	{
		mUpdatees.emplace_back(aEventsBitset, std::move(aUpdatee), aTtl);
	}

}