#include "updater.hpp"

namespace avk
{
	void update_operations_data::operator()(avk::graphics_pipeline& u)
	{
		auto newPipeline = context().create_graphics_pipeline_from_template(*u, [&ed = mEventData](avk::graphics_pipeline_t& aPreparedPipeline){
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

	void update_operations_data::operator()(avk::compute_pipeline& u)
	{
		auto newPipeline = context().create_compute_pipeline_from_template(*u, [&ed = mEventData](avk::compute_pipeline_t& aPreparedPipeline){
			// TODO: Something to alter here?
		});
		newPipeline.enable_shared_ownership(); // Must be, otherwise updater can't handle it.
		std::swap(*newPipeline, *u);
		mUpdateeToCleanUp = std::move(newPipeline); // new == old by now
	}

	void update_operations_data::operator()(avk::ray_tracing_pipeline& u)
	{
		auto newPipeline = context().create_ray_tracing_pipeline_from_template(*u, [&ed = mEventData](avk::ray_tracing_pipeline_t& aPreparedPipeline){
			// TODO: Something to alter here?
		});
		newPipeline.enable_shared_ownership(); // Must be, otherwise updater can't handle it.
		std::swap(*newPipeline, *u);
		mUpdateeToCleanUp = std::move(newPipeline); // new == old by now
	}

	void update_operations_data::operator()(avk::image& u)
	{
		auto newImage = context().create_image_from_template(*u, [&ed = mEventData](avk::image_t& aPreparedImage) {
			if (aPreparedImage.depth() == 1u) {
				const auto newExtent = ed.get_extent_for_old_extent(vk::Extent2D{ aPreparedImage.width(), aPreparedImage.height() });
				aPreparedImage.create_info().extent.width  = newExtent.width;
				aPreparedImage.create_info().extent.height = newExtent.height;
			}
			else {
				LOG_WARNING(std::format("No idea how to update a 3D image with dimensions {}x{}x{}", aPreparedImage.width(), aPreparedImage.height(), aPreparedImage.depth()));
			}
		});
		newImage.enable_shared_ownership();
		std::swap(*newImage, *u);
		mUpdateeToCleanUp = std::move(newImage);
	}

	void update_operations_data::operator()(avk::image_view& u)
	{
		auto newImageView = context().create_image_view_from_template(*u, [&ed = mEventData](avk::image_t& aPreparedImage) {
			if (aPreparedImage.depth() == 1u) {
				const auto newExtent = ed.get_extent_for_old_extent(vk::Extent2D{ aPreparedImage.width(), aPreparedImage.height() });
				aPreparedImage.create_info().extent.width  = newExtent.width;
				aPreparedImage.create_info().extent.height = newExtent.height;
			}
			else {
				LOG_WARNING(std::format("No idea how to update a 3D image with dimensions {}x{}x{}", aPreparedImage.width(), aPreparedImage.height(), aPreparedImage.depth()));
			}
		}, [&ed = mEventData](avk::image_view_t& aPreparedImageView) {
			// Nothing to do here
		});
		newImageView.enable_shared_ownership();

		std::swap(*newImageView, *u);
		mUpdateeToCleanUp = std::move(newImageView);
	}

	void update_operations_data::operator()(event_handler_t& u)
	{
		std::visit(
			avk::lambda_overload{
				[    ](std::function<void()>& f) { f(); },
				[this](std::function<void(const avk::graphics_pipeline&)>& f){
					for (auto* p : this->mEventData.mGraphicsPipelinesToBeCleanedUp) {
						f(*p);
					}
				},
				[this](std::function<void(const avk::compute_pipeline&)>& f) {
					for (auto* p : this->mEventData.mComputePipelinesToBeCleanedUp) {
						f(*p);
					}
				},
				[this](std::function<void(const avk::ray_tracing_pipeline&)>& f) {
					for (auto* p : this->mEventData.mRayTracingPipelinesToBeCleanedUp) {
						f(*p);
					}
				},
				[this](std::function<void(const avk::image&)>& f) {
					for (auto* p : this->mEventData.mImagesToBeCleanedUp) {
						f(*p);
					}
				},
				[this](std::function<void(const avk::image_view&)>& f) {
					for (auto* p : this->mEventData.mImageViewsToBeCleanedUp) {
						f(*p);
					}
				},
			},
			u
		);
	}

	void updater::apply()
	{
		event_data eventData;

		// See if we have any resources to clean up:
		size_t cleanupFrontCount = 0;
		if (!mUpdateesToCleanUp.empty()) {
			// If there are different TTL values, it might happen that some resources are not cleaned at the earliest time, but a bit later.
			auto cleanupIt = std::upper_bound(
				std::begin(mUpdateesToCleanUp), std::end(mUpdateesToCleanUp),
				mCurrentUpdaterFrame, [](window::frame_id_t fid, const auto& tpl) {
					return fid < std::get<window::frame_id_t>(tpl);
				}
			);

			for (auto it = std::begin(mUpdateesToCleanUp); it != cleanupIt; ++it) {
				std::visit(
					avk::lambda_overload{
						[&eventData](avk::graphics_pipeline& aGraphicsPipeline) { eventData.mGraphicsPipelinesToBeCleanedUp.push_back(&aGraphicsPipeline);  },
						[&eventData](avk::compute_pipeline& aComputePipeline) { eventData.mComputePipelinesToBeCleanedUp.push_back(&aComputePipeline); },
						[&eventData](avk::ray_tracing_pipeline& aRayTracingPipeline) { eventData.mRayTracingPipelinesToBeCleanedUp.push_back(&aRayTracingPipeline); },
						[&eventData](avk::image& aImage) { eventData.mImagesToBeCleanedUp.push_back(&aImage); },
						[&eventData](avk::image_view& aImageView) { eventData.mImageViewsToBeCleanedUp.push_back(&aImageView); },
						[](event_handler_t& aEventHandler) { }
					},
					std::get<updatee_t>(*it)
				);
			}

			cleanupFrontCount = std::distance(std::begin(mUpdateesToCleanUp), cleanupIt);
		}

		// Then perform the individual updates:
		//   (See which events have fired)
		uint64_t eventsFired = 0;
		assert(cMaxEvents >= mEvents.size());
		const auto n = std::min(cMaxEvents, mEvents.size());
		for (size_t i = 0; i < n; ++i) {
			bool fired = std::visit(
				avk::lambda_overload{
					[&eventData](std::shared_ptr<event>& e) { return e->update(eventData); },
					[&eventData](files_changed_event& e) { return e.update(eventData); },
					[&eventData](swapchain_changed_event& e) { return e.update(eventData); },
					[&eventData](swapchain_resized_event& e) { return e.update(eventData); },
					[&eventData](swapchain_format_changed_event& e) { return e.update(eventData); },
					[&eventData](concurrent_frames_count_changed_event& e) { return e.update(eventData); },
					[&eventData](swapchain_additional_attachments_changed_event& e) { return e.update(eventData); },
					[&eventData](destroying_graphics_pipeline_event& e) { return e.update(eventData); },
					[&eventData](destroying_compute_pipeline_event& e) { return e.update(eventData); },
					[&eventData](destroying_ray_tracing_pipeline_event& e) { return e.update(eventData); },
					[&eventData](destroying_image_event& e) { return e.update(eventData); },
					[&eventData](destroying_image_view_event& e) { return e.update(eventData); }
				},
				mEvents[i]
			);
			if (fired) {
				eventsFired |= (uint64_t{1} << i);
			}
		}

		// Update all who had at least one of their relevant events fired:
		for (auto& tpl : mUpdatees) {
			bool needsUpdate = (std::get<uint64_t>(tpl) & eventsFired) != 0;
			if (needsUpdate) {
				update_operations_data recreator{eventData, {}};
				std::visit(recreator, std::get<updatee_t>(tpl));
				if (recreator.mUpdateeToCleanUp.has_value()) {
					// This invalidates iterators of the deque, but it's okay, we have saved the cleanupFrontCount and don't need any iterators to be preserved.
					mUpdateesToCleanUp.emplace_back(mCurrentUpdaterFrame + std::get<window::frame_id_t>(tpl), std::move(recreator.mUpdateeToCleanUp.value()));
				}
			}
		}

		// Actually clean up (if there is something to clean up):
		mUpdateesToCleanUp.erase(std::begin(mUpdateesToCleanUp), std::begin(mUpdateesToCleanUp) + cleanupFrontCount);

		++mCurrentUpdaterFrame;
	}

	void updater::add_updatee(uint64_t aEventsBitset, updatee_t aUpdatee, window::frame_id_t aTtl)
	{
		mUpdatees.emplace_back(aEventsBitset, std::move(aUpdatee), aTtl);
	}

	void updater::prepare_for_current_frame()
	{
		// perform a static update due to strange FileWatcher behavior :-/
		files_changed_event::update();
	}
}
