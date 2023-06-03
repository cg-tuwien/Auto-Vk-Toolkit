#pragma once

namespace avk
{
	struct old_new_extent
	{
		vk::Extent2D mOldExtent;
		vk::Extent2D mNewExtent;
	};
	
	/** Data gathered by all the event handlers. And to be evaluated by updatees. */
	struct event_data
	{
		auto find_old_extent(vk::Extent2D aOldExtent)
		{
			return std::find_if(std::begin(mExtents), std::end(mExtents), [aOldExtent](const old_new_extent& e){
				return aOldExtent == e.mOldExtent;
			});
		}

		auto find_old_extent(float aOldWidth, float aOldHeight)
		{
			return std::find_if(std::begin(mExtents), std::end(mExtents), [aOldWidth, aOldHeight](const old_new_extent& e){
				return std::abs(aOldWidth  - static_cast<float>(e.mOldExtent.width))  <= 1.2e-7 /* ~machine epsilon */
				    && std::abs(aOldHeight - static_cast<float>(e.mOldExtent.height)) <= 1.2e-7 /* ~machine epsilon */;
			});
		}

		auto& add_or_find_old_extent(vk::Extent2D aOldExtent)
		{
			auto it = find_old_extent(aOldExtent);
			if (std::end(mExtents) != it) {
				return *it;
			}
			return mExtents.emplace_back(old_new_extent{ aOldExtent, {} });
		}

		auto get_extent_for_old_extent(vk::Extent2D aOldExtent)
		{
			const auto it = find_old_extent(aOldExtent);
			if (std::end(mExtents) != it) {
				return it->mNewExtent;
			}
			return mSwapchainImageExtent.value_or(aOldExtent);
		}

		auto get_extent_for_old_extent(float aOldWidth, float aOldHeight)
		{
			const auto it = find_old_extent(aOldWidth, aOldHeight);
			if (std::end(mExtents) != it) {
				return std::make_tuple(static_cast<float>(it->mNewExtent.width), static_cast<float>(it->mNewExtent.height));
			}
			if (mSwapchainImageExtent.has_value()) {
				return std::make_tuple(static_cast<float>(mSwapchainImageExtent.value().width), static_cast<float>(mSwapchainImageExtent.value().height));
			}
			return std::make_tuple(aOldWidth, aOldHeight);
		}

		std::vector<old_new_extent> mExtents;
		std::optional<vk::Extent2D> mSwapchainImageExtent;
		std::vector<avk::graphics_pipeline*> mGraphicsPipelinesToBeCleanedUp;
		std::vector<avk::compute_pipeline*> mComputePipelinesToBeCleanedUp;
		std::vector<avk::ray_tracing_pipeline*> mRayTracingPipelinesToBeCleanedUp;
		std::vector<avk::image*> mImagesToBeCleanedUp;
		std::vector<avk::image_view*> mImageViewsToBeCleanedUp;
	};
}
