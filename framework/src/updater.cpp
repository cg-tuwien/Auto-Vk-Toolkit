#include <gvk.hpp>

namespace gvk
{
	swapchain_resized_handler::swapchain_resized_handler(window* aWindow)
		: mWindow{ aWindow }
		, mPrevExtent{ aWindow->swap_chain_extent() }
	{		
	}
	
	swapchain_resized_handler& swapchain_resized_handler::update(avk::graphics_pipeline aPipeline)
	{
		mGraphicsPipelines.emplace_back(aPipeline);
		return *this;
	}

	void swapchain_resized_handler::analyze_and_update()
	{
		auto curFrameId = mWindow->current_frame();
		auto curExtent = mWindow->swap_chain_extent();

		if (mPrevExtent != curExtent) {
			for (auto& tpl : mGraphicsPipelines) {
				auto newPipeline = gvk::context().create_graphics_pipeline_from_template(
					std::get<avk::graphics_pipeline>(tpl),
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

		// See if we have any resources to clean up:
		if (!mGraphicsPipelinesToBeDestroyed.empty()) {
			// It should be safe to assume that all elements added to this vector are ordered w.r.t. the frame numbers where they may be destroyed
			auto it = std::upper_bound(
				std::begin(mGraphicsPipelinesToBeDestroyed), std::end(mGraphicsPipelinesToBeDestroyed),
				curFrameId, [](window::frame_id_t fid, const auto& tpl){
					return fid < std::get<window::frame_id_t>(tpl); 
				}
			);
			mGraphicsPipelinesToBeDestroyed.erase(std::begin(mGraphicsPipelinesToBeDestroyed), it);
		}
		
		mPrevExtent = curExtent;
	}


	swapchain_changed_handler::swapchain_changed_handler(window* aWindow)
		: mWindow{ aWindow }
		, mPrevImageViewHandle{ aWindow->swap_chain_image_view_at_index(0).handle() }
	{		
	}
	
	swapchain_changed_handler& swapchain_changed_handler::update(avk::graphics_pipeline aPipeline)
	{
		mGraphicsPipelines.emplace_back(aPipeline);
		return *this;
	}

	void swapchain_changed_handler::analyze_and_update()
	{
		auto curFrameId = mWindow->current_frame();
		auto curExtent = mWindow->swap_chain_extent();
		auto curImageViewHandle = mWindow->swap_chain_image_view_at_index(0).handle();

		if (mPrevImageViewHandle != curImageViewHandle) {
		//
		//
		//   TODO: vvvv  All this code is mostly copy&pasted from swapchain_resized_handler => REFACTOR!  vvv
		//
		//
			for (auto& tpl : mGraphicsPipelines) {
				auto newPipeline = gvk::context().create_graphics_pipeline_from_template(
					std::get<avk::graphics_pipeline>(tpl),
					[curExtent](avk::graphics_pipeline_t& aPreparedPipeline){
						for (auto& vp : aPreparedPipeline.viewports()) {
							vp.width = curExtent.width; // TODO: Update ALL viewports? Can that be a good idea? Probably not... Q: How to find out what exactly to update?
							vp.height = curExtent.height;
						}
						for (auto& sc : aPreparedPipeline.scissors()) {
							sc.extent = curExtent; // TODO: Update ALL scissors? Can that be a good idea? Probably not... Q: How to find out what exactly to update?
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
		
		// See if we have any resources to clean up:
		if (!mGraphicsPipelinesToBeDestroyed.empty()) {
			// It should be safe to assume that all elements added to this vector are ordered w.r.t. the frame numbers where they may be destroyed
			auto it = std::upper_bound(
				std::begin(mGraphicsPipelinesToBeDestroyed), std::end(mGraphicsPipelinesToBeDestroyed),
				curFrameId, [](window::frame_id_t fid, const auto& tpl){
					return fid < std::get<window::frame_id_t>(tpl); 
				}
			);
			mGraphicsPipelinesToBeDestroyed.erase(std::begin(mGraphicsPipelinesToBeDestroyed), it);
		}
		//
		//
		//   TODO: ^^^^  All this code is just copy&pasted from swapchain_resized_handler => REFACTOR!  ^^^^
		//
		//
		
		mPrevImageViewHandle = curImageViewHandle;
	}

	shader_files_changed_handler::shader_files_changed_handler(std::vector<std::string> aPathsToWatch)
		: mMaxConcurrentFrames{ 1 }
		, mFrameCounter{ 0 }
		, mPathsToWatch{ std::move(aPathsToWatch) }
	{
		// Just iterate through all the windows to find out the maximum number of concurrent frames for our lifetime handling:
		context().execute_for_each_window([this](window* w){
			mMaxConcurrentFrames = std::max(mMaxConcurrentFrames, w->number_of_frames_in_flight());
		});
		
		// File watcher operates on folders, not files => get all unique folders
		std::set<std::string> uniqueFolders;
		for (const auto& file : mPathsToWatch)
		{
			auto folder = avk::extract_base_path(file);
			uniqueFolders.insert(folder);
		}
		// ...and pass them to the file watcher
		for (const auto& folder : uniqueFolders)
		{
			mWatchIds.push_back(mFileWatcher.addWatch(folder, this));
		}
	}

	shader_files_changed_handler::~shader_files_changed_handler()
	{
		for (const auto& watchId : mWatchIds)
		{
			mFileWatcher.removeWatch(watchId);
		}
	}
		
	shader_files_changed_handler& shader_files_changed_handler::update(avk::graphics_pipeline aPipeline)
	{
		mGraphicsPipelines.emplace_back(aPipeline);
		return *this;	
	}

	void shader_files_changed_handler::analyze_and_update()
	{
		mFileWatcher.update();

		auto curFrameId = mFrameCounter;
		
		// See if we have any resources to clean up:
		if (!mGraphicsPipelinesToBeDestroyed.empty()) {
			// It should be safe to assume that all elements added to this vector are ordered w.r.t. the frame numbers where they may be destroyed
			auto it = std::upper_bound(
				std::begin(mGraphicsPipelinesToBeDestroyed), std::end(mGraphicsPipelinesToBeDestroyed),
				curFrameId, [](window::frame_id_t fid, const auto& tpl){
					return fid < std::get<window::frame_id_t>(tpl); 
				}
			);
			mGraphicsPipelinesToBeDestroyed.erase(std::begin(mGraphicsPipelinesToBeDestroyed), it);
		}

		++mFrameCounter;
	}

	void shader_files_changed_handler::handleFileAction(FW::WatchID watchid, const FW::String& dir, const FW::String& filename, FW::Action action)
	{
		if (FW::Actions::Modified == action)
		{
			LOG_INFO(fmt::format("File '{}' in directory '{}' has been modified => going to re-create pipelines.", filename, dir));
			
			for (auto& tpl : mGraphicsPipelines) {
				auto newPipeline = gvk::context().create_graphics_pipeline_from_template(std::get<avk::graphics_pipeline>(tpl));
				newPipeline.enable_shared_ownership(); // Must be, otherwise updater can't handle it.
				std::swap(*newPipeline, *std::get<avk::graphics_pipeline>(tpl));
				// bye bye
				mGraphicsPipelinesToBeDestroyed.emplace_back(mFrameCounter + mMaxConcurrentFrames, std::move(newPipeline)); // new == old by now
			}
		}
		// don't care about the other cases
	}

	void updater::render()
	{
		for (auto& handler : mSwapchainResizedHandlers) {
			handler.analyze_and_update();
		}
		for (auto& handler : mSwapchainChangedHandlers) {
			handler.analyze_and_update();
		}
		for (auto& handler : mShaderFilesChangedHandlers) {
			handler.analyze_and_update();
		}
	}
	
	swapchain_resized_handler& updater::on_swapchain_resized(window* aWindow)
	{
		return mSwapchainResizedHandlers.emplace_back(aWindow);
	}

	swapchain_changed_handler& updater::on_swapchain_changed(window* aWindow)
	{
		return mSwapchainChangedHandlers.emplace_back(aWindow);
	}

}