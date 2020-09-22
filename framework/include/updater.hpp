#pragma once
#include <gvk.hpp>

namespace gvk
{
	// Helper-class used in class updater that handles updates after "swapchain resized" events
	class swapchain_resized_handler
	{
	public:
		swapchain_resized_handler(window* aWindow);
		
		// Request updated for pipelines (to be used during setup):
		swapchain_resized_handler& update(avk::graphics_pipeline aPipeline, std::vector<avk::binding_data> aBindingsTodo);

		// To be called each frame:
		void analyze_and_update();

	private:
		window* mWindow;
		vk::Extent2D mPrevExtent;
		// Lists of active resources that are handled and updated
		std::vector<std::tuple<avk::graphics_pipeline, std::vector<avk::binding_data>>> mGraphicsPipelines;

		// Tuples of <frame-id, resource> where the frame-id indicates in which frame a resource shall be destroyed
		std::vector<std::tuple<gvk::window::frame_id_t, avk::graphics_pipeline>> mGraphicsPipelinesToBeDestroyed;
	};

	// An updater that can handle different types of updates
	class updater : public invokee
	{
	public:
		// Make sure, an updater runs first in render()
		int execution_order() const override { return std::numeric_limits<int>::min(); }
		
		void render() override;

		swapchain_resized_handler& on_swapchain_resized(window* aWindow);

	private:
		std::vector<swapchain_resized_handler> mSwapchainResizedHandlers;
	};
}
