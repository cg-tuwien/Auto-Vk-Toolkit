#pragma once
#include <gvk.hpp>
#include <FileWatcher/FileWatcher.h>

namespace gvk
{
	// TODO: Design-wise, these *_handler classes are somewhat ugly and perform the same work multiple times
	//       It would probably be better to refactor the whole code so that it is not like the current version:
	//
	//			.on_swapchain_resized(which)
	//				.update(what)
	//				.update(what_else)
	//
	//       But instead like this:
	//
	//			.update(what)
	//				.on_swapchain_resized(which);
	//			.update(what
	//				.on_swapchain_resized(which);
	//
	//		The point is to have the actual updater-code within class updater and not multiple times
	//		in the separate *_handler classes.
	//		There's a good chance that there are even better approaches.
	//		


	
	// Helper-class used in class updater that handles updates after "swapchain resized" events
	class swapchain_resized_handler
	{
	public:
		swapchain_resized_handler(window* aWindow);
		
		// Request updated for pipelines (to be used during setup):
		swapchain_resized_handler& update(avk::graphics_pipeline aPipeline);

		// To be called each frame:
		void analyze_and_update();

	private:
		window* mWindow;
		vk::Extent2D mPrevExtent;
		// Lists of active resources that are handled and updated
		std::vector<std::tuple<avk::graphics_pipeline>> mGraphicsPipelines;

		// Tuples of <frame-id, resource> where the frame-id indicates in which frame a resource shall be destroyed
		std::deque<std::tuple<gvk::window::frame_id_t, avk::graphics_pipeline>> mGraphicsPipelinesToBeDestroyed;
	};

	// Helper-class used in class updater that handles updates after "swapchain changed" events
	class swapchain_changed_handler
	{
	public:
		swapchain_changed_handler(window* aWindow);
		
		// Request updated for pipelines (to be used during setup):
		swapchain_changed_handler& update(avk::graphics_pipeline aPipeline);

		// To be called each frame:
		void analyze_and_update();

	private:
		window* mWindow;
		VkImageView mPrevImageViewHandle;
		
		// Lists of active resources that are handled and updated
		std::vector<std::tuple<avk::graphics_pipeline>> mGraphicsPipelines;

		// Tuples of <frame-id, resource> where the frame-id indicates in which frame a resource shall be destroyed
		std::deque<std::tuple<gvk::window::frame_id_t, avk::graphics_pipeline>> mGraphicsPipelinesToBeDestroyed;
	};

	// Helper-class used in class updater that handles updates after "shader files changed" events
	class shader_files_changed_handler : public FW::FileWatchListener
	{
	public:
		shader_files_changed_handler(std::vector<std::string> aPathsToWatch);
		~shader_files_changed_handler();
		
		// Request updated for pipelines (to be used during setup):
		shader_files_changed_handler& update(avk::graphics_pipeline aPipeline);

		// To be called each frame:
		void analyze_and_update();

		void handleFileAction(FW::WatchID watchid, const FW::String& dir, const FW::String& filename, FW::Action action) override;
	private:
		window::frame_id_t mMaxConcurrentFrames;
		window::frame_id_t mFrameCounter;
		std::vector<std::string> mPathsToWatch;
		
		// Lists of active resources that are handled and updated
		std::vector<std::tuple<avk::graphics_pipeline>> mGraphicsPipelines;

		// Tuples of <frame-id, resource> where the frame-id indicates in which frame a resource shall be destroyed
		std::deque<std::tuple<gvk::window::frame_id_t, avk::graphics_pipeline>> mGraphicsPipelinesToBeDestroyed;

		FW::FileWatcher mFileWatcher;
		std::vector<FW::WatchID> mWatchIds;
	};

	// An updater that can handle different types of updates
	class updater : public invokee
	{
	public:
		// Make sure, an updater runs first in render()
		int execution_order() const override { return std::numeric_limits<int>::min(); }
		
		void render() override;

		swapchain_resized_handler& on_swapchain_resized(window* aWindow);
		swapchain_changed_handler& on_swapchain_changed(window* aWindow);

		template <typename T>
		shader_files_changed_handler& on_shader_files_changed(const T& aPipeline)
		{
			std::vector<std::string> shaderLoadPaths;
			const auto& shaders = aPipeline.shaders();
			for (const auto& shader : shaders) {
				shaderLoadPaths.emplace_back(shader.actual_load_path());
			}
			return mShaderFilesChangedHandlers.emplace_back(std::move(shaderLoadPaths));
		}

	private:
		std::vector<swapchain_resized_handler> mSwapchainResizedHandlers;
		std::vector<swapchain_changed_handler> mSwapchainChangedHandlers;
		std::deque<shader_files_changed_handler> mShaderFilesChangedHandlers;
	};
}
