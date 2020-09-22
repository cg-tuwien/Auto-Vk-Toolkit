#include <gvk.hpp>

namespace gvk
{
	swapchain_resized_handler::swapchain_resized_handler(window* aWindow)
		: mWindow{ aWindow }
		, mPrevExtent{ aWindow->swap_chain_extent() }
	{		
	}
	
	swapchain_resized_handler& swapchain_resized_handler::update(avk::graphics_pipeline aPipeline, std::vector<avk::binding_data> aBindingsTodo)
	{
		mGraphicsPipelines.emplace_back(aPipeline, aBindingsTodo);
		return *this;
	}

	void swapchain_resized_handler::analyze_and_update()
	{
		auto& curImageAvailableSemaphore = mWindow->current_image_available_semaphore();
		auto curFrameId = mWindow->current_frame();
		auto curExtent = mWindow->swap_chain_extent();

		if (mPrevExtent != curExtent) {
			for (auto& tpl : mGraphicsPipelines) {
				auto newPipeline = gvk::context().create_graphics_pipeline_from_template(
					std::get<avk::graphics_pipeline>(tpl),
					std::get<std::vector<avk::binding_data>>(tpl),
					[prevExtent = mPrevExtent, curExtent](avk::graphics_pipeline_t& aPreparedPipeline){
						const float oldWidth = prevExtent.width;
						const float oldHeight = prevExtent.height;
						for (auto& vp : aPreparedPipeline.viewports()) {
							if (std::abs(oldWidth - vp.width) < std::numeric_limits<float>::epsilon() && std::abs(oldHeight - vp.height) < std::numeric_limits<float>::epsilon()) {
								vp.width = curExtent.width;
								vp.height = curExtent.height;
							}
						}
						for (auto& sc : aPreparedPipeline.scissors()) {
							if (sc.extent == prevExtent) {
								sc.extent = curExtent;
							}
						}
					}
				);
				newPipeline.enable_shared_ownership(); // Must be, otherwise updater can't handle it.
				std::swap(*newPipeline, *std::get<avk::graphics_pipeline>(tpl));
				// bye bye
				mGraphicsPipelinesToBeDestroyed.emplace_back(mWindow->current_frame() + mWindow->number_of_frames_in_flight(), std::move(newPipeline)); // new == old by now
				// TODO: delete from mGraphicsPipelinesToBeDestroyed in #mWindow->number_of_frames_in_flight() frames into the future
			}
		}
		mPrevExtent = curExtent;
	}

	void updater::render()
	{
		for (auto& handler : mSwapchainResizedHandlers) {
			handler.analyze_and_update();
		}
	}
	
	swapchain_resized_handler& updater::on_swapchain_resized(window* aWindow)
	{
		return mSwapchainResizedHandlers.emplace_back(aWindow);
	}
}