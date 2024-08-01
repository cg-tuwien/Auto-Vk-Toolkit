#include "context_vulkan.hpp"

namespace avk
{
	std::mutex window::sSubmitMutex;

	void window::enable_resizing(bool aEnable)
	{
		mShallBeResizable = aEnable;

		if (is_alive()) {
			context().dispatch_to_main_thread([this, aEnable]() {
				glfwSetWindowAttrib(mHandle.value().mHandle, GLFW_RESIZABLE, aEnable ? GLFW_TRUE : GLFW_FALSE);
			});
		}
	}


	void window::request_srgb_framebuffer(bool aRequestSrgb)
	{
		// Which formats are supported, depends on the surface.
		mSurfaceFormatSelector = [lSrgbFormatRequested = aRequestSrgb](const vk::SurfaceKHR & surface) {
			// Get all the formats which are supported by the surface:
			auto srfFrmts = context().physical_device().getSurfaceFormatsKHR(surface);

			// Init with a default format...
			auto selSurfaceFormat = vk::SurfaceFormatKHR{
				vk::Format::eB8G8R8A8Unorm,
				vk::ColorSpaceKHR::eSrgbNonlinear
			};

			// ...and try to possibly find one which is definitely supported or better suited w.r.t. the surface.
			if (!(srfFrmts.size() == 1 && srfFrmts[0].format == vk::Format::eUndefined)) {
				for (const auto& e : srfFrmts) {
					if (lSrgbFormatRequested) {
						if (avk::is_srgb_format(e.format)) {
							selSurfaceFormat = e;
							break;
						}
					}
					else {
						if (!avk::is_srgb_format(e.format)) {
							selSurfaceFormat = e;
							break;
						}
					}
				}
			}

			// In any case, return a format
			return selSurfaceFormat;
		};

		if (is_alive()) {
			mResourceRecreationDeterminator.set_recreation_required_for(recreation_determinator::reason::image_format_or_properties_changed);
		}
	}

	void window::set_presentaton_mode(avk::presentation_mode aMode)
	{
		mPresentationModeSelector = [lPresMode = aMode](const vk::SurfaceKHR& surface) {
			// Supported presentation modes must be queried from a device:
			auto presModes = context().physical_device().getSurfacePresentModesKHR(surface);

			// Select a presentation mode:
			auto selPresModeItr = presModes.end();
			switch (lPresMode) {
			case avk::presentation_mode::immediate:
				selPresModeItr = std::find(std::begin(presModes), std::end(presModes), vk::PresentModeKHR::eImmediate);
				break;
			case avk::presentation_mode::relaxed_fifo:
				selPresModeItr = std::find(std::begin(presModes), std::end(presModes), vk::PresentModeKHR::eFifoRelaxed);
				break;
			case avk::presentation_mode::fifo:
				selPresModeItr = std::find(std::begin(presModes), std::end(presModes), vk::PresentModeKHR::eFifo);
				break;
			case avk::presentation_mode::mailbox:
				selPresModeItr = std::find(std::begin(presModes), std::end(presModes), vk::PresentModeKHR::eMailbox);
				break;
			default:
				throw avk::runtime_error("should not get here");
			}
			if (selPresModeItr == presModes.end()) {
				LOG_WARNING_EM("No presentation mode specified or desired presentation mode not available => will select any presentation mode");
				selPresModeItr = presModes.begin();
			}

			return *selPresModeItr;
		};

		// If the window has already been created, the new setting can't
		// be applied unless the swapchain is being recreated.
		if (is_alive()) {
			mResourceRecreationDeterminator.set_recreation_required_for(recreation_determinator::reason::presentation_mode_changed);
		}
	}

	void window::set_number_of_presentable_images(uint32_t aNumImages)
	{
		mNumberOfPresentableImagesGetter = [lNumImages = aNumImages]() { return lNumImages; };

		// If the window has already been created, the new setting can't
		// be applied unless the swapchain is being recreated.
		if (is_alive()) {
			mResourceRecreationDeterminator.set_recreation_required_for(recreation_determinator::reason::presentable_images_count_changed);
		}
	}

	void window::set_number_of_concurrent_frames(window::frame_id_t aNumConcurrent)
	{
		mNumberOfConcurrentFramesGetter = [lNumConcurrent = aNumConcurrent]() { return lNumConcurrent; };

		// If the window has already been created, the new setting can't
		// be applied unless synchronization infrastructure is being recreated.
		if (is_alive()) {
			mResourceRecreationDeterminator.set_recreation_required_for(recreation_determinator::reason::concurrent_frames_count_changed);
		}
	}

	void window::set_additional_back_buffer_attachments(std::vector<avk::attachment> aAdditionalAttachments)
	{
		mAdditionalBackBufferAttachmentsGetter = [lAdditionalAttachments = std::move(aAdditionalAttachments)]() { return lAdditionalAttachments; };

		// If the window has already been created, the new setting can't
		// be applied unless the swapchain is being recreated.
		if (is_alive()) {
			mResourceRecreationDeterminator.set_recreation_required_for(recreation_determinator::reason::backbuffer_attachments_changed);
		}
	}

	void window::set_image_usage_properties(avk::image_usage aImageUsageProperties)
	{
		mImageUsageGetter = [lImageUsageProperties = aImageUsageProperties]() { return lImageUsageProperties; };

		// If the window has already been created, the new setting can't
		// be applied unless the swapchain is being recreated.
		if (is_alive()) {
			mResourceRecreationDeterminator.set_recreation_required_for(recreation_determinator::reason::image_format_or_properties_changed);
		}
	}

	avk::image_usage window::get_config_image_usage_properties()
	{
		if (!mImageUsageGetter) {
			// Set the default:
			set_image_usage_properties(avk::image_usage::color_attachment | avk::image_usage::transfer_destination | avk::image_usage::transfer_source | avk::image_usage::presentable | avk::image_usage::tiling_optimal);
		}
		// Determine the image usage flags:
		return mImageUsageGetter();
	}

	void window::open()
	{
		context().dispatch_to_main_thread([this]() {
			// Ensure, previous work is done:
			context().work_off_event_handlers();

			// Share the graphics context between all windows
			auto* sharedContex = context().get_window_for_shared_context();
			// Bring window into existance:
			glfwWindowHint(GLFW_RESIZABLE, get_config_shall_be_resizable() ? GLFW_TRUE : GLFW_FALSE);
			auto* handle = glfwCreateWindow(mRequestedSize.mWidth, mRequestedSize.mHeight,
				mTitle.c_str(),
				mMonitor.has_value() ? mMonitor->mHandle : nullptr,
				sharedContex);
			if (nullptr == handle) {
				// No point in continuing
				throw avk::runtime_error("Failed to create window with the title '" + mTitle + "'");
			}
			mHandle = window_handle{ handle };
			initialize_after_open();

			// There will be some pending work regarding this newly created window stored within the
			// context's events, like creating a swap chain and so on.
			// Why wait? Invoke them now!
			context().work_off_event_handlers();
		});
	}

	bool window::get_config_shall_be_resizable() const
	{
		return mShallBeResizable;
	}

	vk::SurfaceFormatKHR window::get_config_surface_format(const vk::SurfaceKHR & aSurface)
	{
		if (!mSurfaceFormatSelector) {
			// Set the default:
			request_srgb_framebuffer(false);
		}
		// Determine the format:
		return mSurfaceFormatSelector(aSurface);
	}

	vk::PresentModeKHR window::get_config_presentation_mode(const vk::SurfaceKHR & aSurface)
	{
		if (!mPresentationModeSelector) {
			// Set the default:
			set_presentaton_mode(avk::presentation_mode::mailbox);
		}
		// Determine the presentation mode:
		return mPresentationModeSelector(aSurface);
	}

	uint32_t window::get_config_number_of_presentable_images()
	{
		if (!mNumberOfPresentableImagesGetter) {
			auto srfCaps = context().physical_device().getSurfaceCapabilitiesKHR(surface());
			auto imageCount = srfCaps.minImageCount + 1u;
			if (srfCaps.maxImageCount > 0) { // A value of 0 for maxImageCount means that there is no limit
				imageCount = glm::min(imageCount, srfCaps.maxImageCount);
			}
			return imageCount;
		}
		return mNumberOfPresentableImagesGetter();
	}

	window::frame_id_t window::get_config_number_of_concurrent_frames()
	{
		if (!mNumberOfConcurrentFramesGetter) {
			return get_config_number_of_presentable_images();
		}
		return mNumberOfConcurrentFramesGetter();
	}

	std::vector<avk::attachment> window::get_additional_back_buffer_attachments()
	{
		if (!mAdditionalBackBufferAttachmentsGetter) {
			return {};
		}
		else {
			return mAdditionalBackBufferAttachmentsGetter();
		}
	}

	//void window::handle_lifetime(avk::command_buffer aCommandBuffer, std::optional<frame_id_t> aFrameId)
	//{
	//	std::scoped_lock<std::mutex> guard(sSubmitMutex); // Protect against concurrent access from invokees
	//	if (!aFrameId.has_value()) {
	//		aFrameId = current_frame();
	//	}

	//	aCommandBuffer->invoke_post_execution_handler(); // Yes, do it now!

	//	auto& refTpl = mLifetimeHandledCommandBuffers.emplace_back(aFrameId.value(), std::move(aCommandBuffer));
	//	// ^ Prefer code duplication over recursive_mutex
	//}

    void window::handle_lifetime(any_window_resource_t aResource, std::optional<frame_id_t> aFrameId)
    {
        std::scoped_lock<std::mutex> guard(sSubmitMutex); // Protect against concurrent access from invokees
        if (!aFrameId.has_value()) {
            aFrameId = current_frame();
        }

		if (std::holds_alternative<command_buffer>(aResource)) {
            std::get<command_buffer>(aResource)->invoke_post_execution_handler(); // Yes, do it now!
		}

        mLifetimeHandledResources.emplace_back(aFrameId.value(), std::move(aResource));
    }

	//void window::handle_lifetime(outdated_swapchain_resource_t&& aOutdatedSwapchain, std::optional<frame_id_t> aFrameId)
	//{
	//	std::scoped_lock<std::mutex> guard(sSubmitMutex); // Protect against concurrent access from invokees
	//	if (!aFrameId.has_value()) {
	//		aFrameId = current_frame();
	//	}
	//	mOutdatedSwapChainResources.emplace_back(aFrameId.value(), std::move(aOutdatedSwapchain));
	//}

	std::vector<avk::semaphore> window::remove_all_present_semaphore_dependencies_for_frame(frame_id_t aPresentFrameId)
	{
		// No need to protect against concurrent access since that would be misuse of this function.
		// This shall never be called from the invokee callbacks as being invoked through a parallel invoker.

		// Find all to remove
		auto to_remove = std::remove_if(
			std::begin(mPresentSemaphoreDependencies), std::end(mPresentSemaphoreDependencies),
			[maxTTL = aPresentFrameId - number_of_frames_in_flight()](const auto& tpl) {
				return std::get<frame_id_t>(tpl) <= maxTTL;
			});
		// return ownership of all the semaphores to remove to the caller
		std::vector<avk::semaphore> moved_semaphores;
		for (decltype(to_remove) it = to_remove; it != std::end(mPresentSemaphoreDependencies); ++it) {
			moved_semaphores.push_back(std::move(std::get<avk::semaphore>(*it)));
		}
		// Erase and return
		mPresentSemaphoreDependencies.erase(to_remove, std::end(mPresentSemaphoreDependencies));
		return moved_semaphores;
	}

	std::vector<window::any_window_resource_t> window::clean_up_resources_for_frame(frame_id_t aPresentFrameId)
	{
        std::vector<any_window_resource_t> deadResources;
		if (mLifetimeHandledResources.empty()) {
			return deadResources;
		}
		// No need to protect against concurrent access since that would be misuse of this function.
		// This shall never be called from the invokee callbacks as being invoked through a parallel invoker.

		// Up to the frame with id 'maxTTL', all command buffers can be safely removed
		const auto maxTTL = aPresentFrameId - number_of_frames_in_flight();

		// 1. SINGLE USE COMMAND BUFFERS
		// Can not use the erase-remove idiom here because that would invalidate iterators and references
		// HOWEVER: "[...]unless the erased elements are at the end or the beginning of the container,
		// in which case only the iterators and references to the erased elements are invalidated." => Let's do that!
        auto eraseBegin = std::begin(mLifetimeHandledResources);
        if (std::end(mLifetimeHandledResources) == eraseBegin || std::get<frame_id_t>(*eraseBegin) > maxTTL) {
			return deadResources;
		}
		// There are elements that we can remove => find position until where:
		auto eraseEnd = eraseBegin;
        while (eraseEnd != std::end(mLifetimeHandledResources) && std::get<frame_id_t>(*eraseEnd) <= maxTTL) {
			// return ownership of all the command_buffers to remove to the caller
            deadResources.push_back(std::move(std::get<any_window_resource_t>(*eraseEnd)));
			++eraseEnd;
		}
        mLifetimeHandledResources.erase(eraseBegin, eraseEnd);
		return deadResources;
	}

	//void window::clean_up_outdated_swapchain_resources_for_frame(frame_id_t aPresentFrameId)
	//{
	//	if (mOutdatedSwapChainResources.empty()) {
	//		return;
	//	}

	//	// Up to the frame with id 'maxTTL', all swap chain resources can be safely removed
	//	const auto maxTTL = aPresentFrameId - number_of_frames_in_flight();

	//	auto eraseBegin = std::begin(mOutdatedSwapChainResources);
	//	if (std::end(mOutdatedSwapChainResources) == eraseBegin || std::get<frame_id_t>(*eraseBegin) > maxTTL) {
	//		return;
	//	}
	//	auto eraseEnd = eraseBegin;
	//	while (eraseEnd != std::end(mOutdatedSwapChainResources) && std::get<frame_id_t>(*eraseEnd) <= maxTTL) {
	//		++eraseEnd;
	//	}
	//	mOutdatedSwapChainResources.erase(eraseBegin, eraseEnd);
	//}

	void window::fill_in_present_semaphore_dependencies_for_frame(std::vector<vk::Semaphore>& aSemaphores, frame_id_t aFrameId) const
	{
		for (const auto& [frameId, sem] : mPresentSemaphoreDependencies) {
			if (frameId == aFrameId) {
				auto& info = aSemaphores.emplace_back(sem->handle());
			}
		}
	}

	void window::acquire_next_swap_chain_image_and_prepare_semaphores()
	{
		if (mResourceRecreationDeterminator.is_any_recreation_necessary())	{
			if (mResourceRecreationDeterminator.has_concurrent_frames_count_changed_only())	{ //only update framesInFlight fences/semaphores
				mActivePresentationQueue->handle().waitIdle(); // ensure the semaphores which we are going to potentially delete are not being used
				update_concurrent_frame_synchronization(swapchain_creation_mode::update_existing_swapchain);
				mResourceRecreationDeterminator.reset();
			}
			else { //recreate the whole swap chain then
				update_resolution_and_recreate_swap_chain();
			}
		}
		// Update the presentation queue only after potential swap chain updates:
		update_active_presentation_queue();

		// Get the next image from the swap chain, GPU -> GPU sync from previous present to the following acquire
		auto imgAvailableSem = image_available_semaphore_for_frame();

		// Update previous image index before getting a new image index for the current frame:
		mPreviousFrameImageIndex = mCurrentFrameImageIndex;

		try
		{
			auto result = context().device().acquireNextImageKHR(
				swap_chain(), // the swap chain from which we wish to acquire an image
				// At this point, I have to rant about the `timeout` parameter:
				// The spec says: "timeout specifies how long the function waits, in nanoseconds, if no image is available."
				// HOWEVER, don't think that a numeric_limit<int64_t>::max() will wait for nine quintillion nanoseconds!
				//    No, instead it will return instantly, yielding an invalid swap chain image index. OMG, WTF?!
				// Long story short: make sure to pass the UNSINGEDint64_t's maximum value, since only that will disable the timeout.
				std::numeric_limits<uint64_t>::max(), // a timeout in nanoseconds for an image to become available. Using the maximum value of a 64 bit unsigned integer disables the timeout. [1]
				imgAvailableSem->handle(), // The next two parameters specify synchronization objects that are to be signaled when the presentation engine is finished using the image [1]
				nullptr,
				&mCurrentFrameImageIndex); // a variable to output the index of the swap chain image that has become available. The index refers to the VkImage in our swapChainImages array. We're going to use that index to pick the right command buffer. [1]
			if (vk::Result::eSuboptimalKHR == result) {
				LOG_INFO("Swap chain is suboptimal in acquire_next_swap_chain_image_and_prepare_semaphores. Going to recreate it...");
				mResourceRecreationDeterminator.set_recreation_required_for(recreation_determinator::reason::suboptimal_swap_chain);

				// Workaround for binary semaphores:
				// Since the semaphore is in a wait state right now, we'll have to wait for it until we can use it again.
				auto fen = context().create_fence();
				vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eAllCommands;
				mActivePresentationQueue->handle().submit({ // TODO: This works, but is arguably not the greatest of all solutions... => Can it be done better with Timeline Semaphores? (Test on AMD!)
					vk::SubmitInfo{}
						.setCommandBufferCount(0u)
						.setWaitSemaphoreCount(1u)
						.setPWaitSemaphores(imgAvailableSem->handle_addr())
						.setPWaitDstStageMask(&waitStage)
				}, fen->handle());
				fen->wait_until_signalled();

				acquire_next_swap_chain_image_and_prepare_semaphores();
				return;
			}
		}
		catch (vk::OutOfDateKHRError omg) {
			LOG_INFO(std::format("Swap chain out of date in acquire_next_swap_chain_image_and_prepare_semaphores. Reason[{}] in frame#{}. Going to recreate it...", omg.what(), current_frame()));
			mResourceRecreationDeterminator.set_recreation_required_for(recreation_determinator::reason::invalid_swap_chain);
			acquire_next_swap_chain_image_and_prepare_semaphores();
			return;
		}

		// It could be that the image index that has been returned is currently in flight.
		// There's no guarantee that we'll always get a nice cycling through the indices.
		// => Must handle this case!
		assert(current_image_index() == mCurrentFrameImageIndex);
		if (mImagesInFlightFenceIndices[current_image_index()] >= 0) {
			LOG_DEBUG_VERBOSE(std::format("Frame #{}: Have to issue an extra fence-wait because swap chain returned image[{}] but fence[{}] is currently in use.", current_frame(), mCurrentFrameImageIndex, mImagesInFlightFenceIndices[current_image_index()]));
			auto& xf = mFramesInFlightFences[mImagesInFlightFenceIndices[current_image_index()]];
			xf->wait_until_signalled();
			// But do not reset! Otherwise we will wait forever at the next wait_until_signalled that will happen for sure.
		}

		// Set the image available semaphore to be consumed:
		mCurrentFrameImageAvailableSemaphore = imgAvailableSem;

		// Set the fence to be used:
		mCurrentFrameFinishedFence = current_fence();
	}

	void window::sync_before_render()
	{
		// Wait for the fence before proceeding, GPU -> CPU synchronization via fence
		const auto ci = current_in_flight_index();
		auto cf = current_fence();
		assert(cf->handle() == mFramesInFlightFences[current_in_flight_index()]->handle());
		cf->wait_until_signalled();
		cf->reset();

		// Keep house with the in-flight images:
		//   However, we don't know which index this fence had been mapped to => we have to search
		for (auto& mapping : mImagesInFlightFenceIndices) {
			if (ci == mapping) {
				mapping = -1;
				break;
			}
		}

		// At this point we are certain that the frame which has used the current fence before is done.
		//  => Clean up the resources of that previous frame!
		auto semaphoresToBeFreed = remove_all_present_semaphore_dependencies_for_frame(current_frame());
		auto commandBuffersToBeFreed 	= clean_up_resources_for_frame(current_frame());
		//clean_up_outdated_swapchain_resources_for_frame(current_frame());

		acquire_next_swap_chain_image_and_prepare_semaphores();
	}

	void window::render_frame()
	{
		const auto fenceIndex = static_cast<int>(current_in_flight_index());

		// EXTERN -> WAIT
		std::vector<vk::Semaphore> waitSemHandles;
		fill_in_present_semaphore_dependencies_for_frame(waitSemHandles, current_frame());
		std::vector<vk::SemaphoreSubmitInfoKHR> waitSemInfos(waitSemHandles.size());
		std::ranges::transform(std::begin(waitSemHandles), std::end(waitSemHandles), std::begin(waitSemInfos), [](const auto& semHandle){
			return vk::SemaphoreSubmitInfoKHR{ semHandle, 0, vk::PipelineStageFlagBits2KHR::eAllCommands }; // TODO: Really ALL_COMMANDS or could we also use NONE?
		});

		if (!has_consumed_current_image_available_semaphore()) {
			// Being in this branch indicates that image available semaphore has not been consumed yet
			// meaning that no render calls were (correctly) executed  (hint: check if all invokess are possibly disabled).
			auto imgAvailable = consume_current_image_available_semaphore();
			waitSemInfos.emplace_back(imgAvailable->handle())
				.setStageMask(vk::PipelineStageFlagBits2KHR::eAllCommands);
		}
		
		if (!has_used_current_frame_finished_fence()) {
			// Need an additional submission to signal the fence.
			auto fence = use_current_frame_finished_fence();
			assert(fence->handle() == mFramesInFlightFences[fenceIndex]->handle());

			// Using a temporary semaphore for the signal operation:
			auto sigSem = context().create_semaphore();
			vk::SemaphoreSubmitInfoKHR sigSemInfo{ sigSem->handle() };
			
			// Waiting on the same semaphores here and during vkPresentKHR should be fine: (TODO: is it?)
			auto submitInfo = vk::SubmitInfo2KHR{}
				.setWaitSemaphoreInfoCount(static_cast<uint32_t>(waitSemInfos.size()))
				.setPWaitSemaphoreInfos(waitSemInfos.data())
				.setCommandBufferInfoCount(0u)    // Submit ZERO command buffers :O
				.setSignalSemaphoreInfoCount(1u)
				.setPSignalSemaphoreInfos(&sigSemInfo);
#ifdef AVK_USE_SYNCHRONIZATION2_INSTEAD_OF_CORE
			auto errorCode = mActivePresentationQueue->handle().submit2KHR(1u, &submitInfo, fence->handle(), context().dispatch_loader_ext());
			if (vk::Result::eSuccess != errorCode) {
				AVK_LOG_WARNING("submit2KHR returned " + vk::to_string(errorCode));
			}
#else
			auto errorCode = mActivePresentationQueue->handle().submit2(1u, &submitInfo, fence->handle(), context().dispatch_loader_core());
			if (vk::Result::eSuccess != errorCode) {
				AVK_LOG_WARNING("submit2 returned " + vk::to_string(errorCode));
			}
#endif

			// Consequently, the present call must wait on the temporary semaphore only:
			waitSemHandles.clear();
			waitSemHandles.emplace_back(sigSem->handle());
			// Add it as dependency to the current frame, so that it gets properly lifetime-handled:
			add_present_dependency_for_current_frame(std::move(sigSem));
		}

		try
		{
			// SIGNAL -> PRESENT
			auto presentInfo = vk::PresentInfoKHR{}
				.setWaitSemaphoreCount(waitSemHandles.size())
				.setPWaitSemaphores(waitSemHandles.data())
				.setSwapchainCount(1u)
				.setPSwapchains(&swap_chain())
				.setPImageIndices(&mCurrentFrameImageIndex)
				.setPResults(nullptr);
			auto result = mActivePresentationQueue->handle().presentKHR(presentInfo);
			
			// Submitted => store the image index for extra reuse-safety:
			mImagesInFlightFenceIndices[mCurrentFrameImageIndex] = fenceIndex;

			if (vk::Result::eSuboptimalKHR == result) {
				LOG_INFO("Swap chain is suboptimal in render_frame. Going to recreate it...");
				mResourceRecreationDeterminator.set_recreation_required_for(recreation_determinator::reason::suboptimal_swap_chain);
				// swap chain will be recreated in the next frame
			}
		}
		catch (const vk::OutOfDateKHRError& omg) {
			LOG_INFO(std::format("Swap chain out of date in render_frame. Reason[{}] in frame#{}. Going to recreate it...", omg.what(), current_frame()));
			mResourceRecreationDeterminator.set_recreation_required_for(recreation_determinator::reason::invalid_swap_chain);
			// Just do nothing. Ignore the failure. This frame is lost.
			// swap chain will be recreated in the next frame
		}

		// increment frame counter
		++mCurrentFrame;
	}

	void window::set_queue_family_ownership(std::vector<uint32_t> aQueueFamilies)
	{
		mQueueFamilyIndicesGetter = [families = std::move(aQueueFamilies)](){ return families; };
		
		// If the window has already been created, the new setting can't
		// be applied unless the swapchain is being recreated.
		if (is_alive()) {
			mResourceRecreationDeterminator.set_recreation_required_for(recreation_determinator::reason::image_format_or_properties_changed);
		}
	}

	void window::set_queue_family_ownership(uint32_t aQueueFamily)
	{
		set_queue_family_ownership(std::vector<uint32_t>{ aQueueFamily });
	}

	void window::set_present_queue(avk::queue& aPresentQueue)
	{
		if (nullptr == mActivePresentationQueue) {
			mActivePresentationQueue = &aPresentQueue;
		}
		else {
			// Update it later, namely after potential swap chain updates have been executed:
			mPresentationQueueGetter = [newPresentQueue = &aPresentQueue]() { return newPresentQueue; };
		}
	}

	void window::update_active_presentation_queue()
	{
		if (mPresentationQueueGetter) {
			mActivePresentationQueue = mPresentationQueueGetter();
			mPresentationQueueGetter = {}; // reset
		}
	}

	void window::create_swap_chain(swapchain_creation_mode aCreationMode)
	{
		if (aCreationMode == window::swapchain_creation_mode::create_new_swapchain) {
			mCurrentFrame = 0; // Start af frame 0
		}

		construct_swap_chain_creation_info(aCreationMode);

		auto lifetimeHandler = [this](vk::UniqueHandle<vk::SwapchainKHR, DISPATCH_LOADER_CORE_TYPE>&& aOldResource) { this->handle_lifetime(std::move(aOldResource)); };
		// assign the new swap chain instead of the old one, if one exists
		avk::assign_and_lifetime_handle_previous(mSwapChain, context().device().createSwapchainKHRUnique(mSwapChainCreateInfo, nullptr, context().dispatch_loader_core()), lifetimeHandler);

		construct_backbuffers(aCreationMode);

		// if we are creating a new swap chain from the ground up, or if the number of concurrent frames change
		// set up fences and other basic initialization
		if (aCreationMode == window::swapchain_creation_mode::create_new_swapchain ||
			mResourceRecreationDeterminator.is_recreation_required_for(recreation_determinator::reason::presentable_images_count_changed) ||
			mResourceRecreationDeterminator.is_recreation_required_for(recreation_determinator::reason::concurrent_frames_count_changed)) {
			update_concurrent_frame_synchronization(aCreationMode);
		}

		mResourceRecreationDeterminator.reset();
	}

	void window::update_concurrent_frame_synchronization(swapchain_creation_mode aCreationMode)
	{
		// ============= SYNCHRONIZATION OBJECTS ===========
			// per CONCURRENT FRAME:

		auto framesInFlight = get_config_number_of_concurrent_frames();

		mFramesInFlightFences.clear();
		mImageAvailableSemaphores.clear();
		mFramesInFlightFences.reserve(framesInFlight);
		mImageAvailableSemaphores.reserve(framesInFlight);
		for (uint32_t i = 0; i < framesInFlight; ++i) {
			mFramesInFlightFences.push_back(context().create_fence(true)); // true => Create the fences in signalled state, so that `avk::context().logical_device().waitForFences` at the beginning of `window::render_frame` is not blocking forever, but can continue immediately.
			mImageAvailableSemaphores.push_back(context().create_semaphore());
		}

		auto imagesInFlight = get_config_number_of_presentable_images();
		mImagesInFlightFenceIndices.clear();
		mImagesInFlightFenceIndices.reserve(imagesInFlight);
		for (uint32_t i = 0; i < imagesInFlight; ++i) {
			mImagesInFlightFenceIndices.push_back(-1);
		}
		assert(mImagesInFlightFenceIndices.size() == imagesInFlight);
		
		// when updating, the current fence must be unsignaled.
		if (aCreationMode == window::swapchain_creation_mode::update_existing_swapchain) {
			current_fence()->reset();
		}

		assert(static_cast<frame_id_t>(mFramesInFlightFences.size()) == get_config_number_of_concurrent_frames());
		assert(static_cast<frame_id_t>(mImageAvailableSemaphores.size()) == get_config_number_of_concurrent_frames());
	}

	void window::update_resolution_and_recreate_swap_chain()
	{
		// Gotta wait until the resolution has been updated on the main thread:
		update_resolution();
		std::atomic_bool resolutionUpdated = false;
		context().dispatch_to_main_thread([&resolutionUpdated]() { resolutionUpdated = true; });
		context().signal_waiting_main_thread();
		mActivePresentationQueue->handle().waitIdle();
		while(!resolutionUpdated) { LOG_DEBUG("Waiting for main thread..."); }

		// If the window is minimized, we've gotta wait even longer:
		while (resolution().x * resolution().y == 0u) {
			LOG_DEBUG(std::format("Waiting for resolution {}x{} to change...", resolution().x, resolution().y));
			context().dispatch_to_main_thread([this](){
				int width = 0, height = 0;
				glfwGetFramebufferSize(handle()->mHandle, &width, &height);
				while (width == 0 || height == 0) {
					glfwGetFramebufferSize(handle()->mHandle, &width, &height);
					glfwWaitEvents();
				}
				update_resolution();
			});
			context().work_off_all_pending_main_thread_actions();
		}

		create_swap_chain(swapchain_creation_mode::update_existing_swapchain);
	}

	std::vector<avk::recorded_commands_t> window::layout_transitions_for_all_backbuffer_images()
	{
		std::vector<avk::recorded_commands_t> result;
		for (auto& bb : mBackBuffers) {
			const auto n = bb->image_views().size();
			assert(n == renderpass_reference().number_of_attachment_descriptions());
			for (size_t i = 0; i < n; ++i) {
				result.push_back(
					avk::sync::image_memory_barrier(bb->image_view_at(i)->get_image(), avk::stage::none >> avk::stage::none)
					.with_layout_transition(avk::layout::undefined >> avk::layout::image_layout{ renderpass_reference().attachment_descriptions()[i].finalLayout })
				);
			}
		}
		return result;
	}

	void window::construct_swap_chain_creation_info(swapchain_creation_mode aCreationMode) {

		auto setOwnership = [this]() {
			// Handle queue family ownership:

			const std::vector<uint32_t> queueFamilyIndices = mQueueFamilyIndicesGetter();

			switch (queueFamilyIndices.size()) {
			case 0:
				throw avk::runtime_error(std::format("You must assign at least set one queue(family) to window '{}'! You can use window::set_queue_family_ownership.", title()));
			case 1:
				mImageCreateInfoSwapChain
					.setSharingMode(vk::SharingMode::eExclusive)
					.setQueueFamilyIndexCount(1u)
					.setPQueueFamilyIndices(&queueFamilyIndices[0]); // could also leave at nullptr!
				break;
			default:
				// Have to use separate queue families!
				// If the queue families differ, then we'll be using the concurrent mode [2]
				mImageCreateInfoSwapChain
					.setSharingMode(vk::SharingMode::eConcurrent)
					.setQueueFamilyIndexCount(static_cast<uint32_t>(queueFamilyIndices.size()))
					.setPQueueFamilyIndices(queueFamilyIndices.data());
				break;
			}
		};

		auto srfCaps = context().physical_device().getSurfaceCapabilitiesKHR(surface());
		auto extent = context().get_resolution_for_window(this);
		auto surfaceFormat = get_config_surface_format(surface());
		if (aCreationMode == swapchain_creation_mode::update_existing_swapchain) {
			mImageCreateInfoSwapChain.setExtent(vk::Extent3D(extent.x, extent.y, 1u));
			if (mResourceRecreationDeterminator.is_recreation_required_for(recreation_determinator::reason::image_format_or_properties_changed)) {
				auto imageUsageProperties = get_config_image_usage_properties();
				auto [imageUsage, imageTiling, createFlags] = avk::to_vk_image_properties(imageUsageProperties);
				mImageCreateInfoSwapChain
					.setFormat(surfaceFormat.format)
					.setUsage(imageUsage)
					.setTiling(imageTiling)
					.setFlags(createFlags);
				setOwnership();
			}
		}
		else {
			auto imageUsageProperties = get_config_image_usage_properties();
			auto [imageUsage, imageTiling, createFlags] = avk::to_vk_image_properties(imageUsageProperties);

			mImageCreateInfoSwapChain = vk::ImageCreateInfo{}
				.setImageType(vk::ImageType::e2D)
				.setFormat(surfaceFormat.format)
				.setExtent(vk::Extent3D(extent.x, extent.y, 1u))
				.setMipLevels(1)
				.setArrayLayers(1)
				.setSamples(vk::SampleCountFlagBits::e1)
				.setTiling(imageTiling)
				.setUsage(imageUsage)
				.setInitialLayout(vk::ImageLayout::eUndefined)
				.setFlags(createFlags);

			setOwnership();
		}

		// With all settings gathered, construct/update swap chain creation info.
		if (aCreationMode == swapchain_creation_mode::update_existing_swapchain) {
			mSwapChainCreateInfo
				.setImageExtent(vk::Extent2D{ mImageCreateInfoSwapChain.extent.width, mImageCreateInfoSwapChain.extent.height })
				.setOldSwapchain(mSwapChain.get());
			if (mResourceRecreationDeterminator.is_recreation_required_for(recreation_determinator::reason::presentation_mode_changed)) {
				mSwapChainCreateInfo.setPresentMode(get_config_presentation_mode(surface()));
			}
			if (mResourceRecreationDeterminator.is_recreation_required_for(recreation_determinator::reason::presentable_images_count_changed)) {
				mSwapChainCreateInfo.setMinImageCount(get_config_number_of_presentable_images());
			}
			if (mResourceRecreationDeterminator.is_recreation_required_for(recreation_determinator::reason::image_format_or_properties_changed)) {
				mSwapChainCreateInfo
					.setImageColorSpace(surfaceFormat.colorSpace)
					.setImageFormat(mImageCreateInfoSwapChain.format)
					.setImageUsage(mImageCreateInfoSwapChain.usage)
					.setImageSharingMode(mImageCreateInfoSwapChain.sharingMode)
					.setQueueFamilyIndexCount(mImageCreateInfoSwapChain.queueFamilyIndexCount)
					.setPQueueFamilyIndices(mImageCreateInfoSwapChain.pQueueFamilyIndices);

				mSwapChainImageFormat = surfaceFormat.format;
			}
		}
		else {
			mSwapChainCreateInfo = vk::SwapchainCreateInfoKHR{}
				.setSurface(surface())
				.setMinImageCount(get_config_number_of_presentable_images())
				.setImageFormat(mImageCreateInfoSwapChain.format)
				.setImageColorSpace(surfaceFormat.colorSpace)
				.setImageExtent(vk::Extent2D{ mImageCreateInfoSwapChain.extent.width, mImageCreateInfoSwapChain.extent.height })
				.setImageArrayLayers(mImageCreateInfoSwapChain.arrayLayers) // The imageArrayLayers specifies the amount of layers each image consists of. This is always 1 unless you are developing a stereoscopic 3D application. [2]
				.setImageUsage(mImageCreateInfoSwapChain.usage)
				.setPreTransform(srfCaps.currentTransform) // To specify that you do not want any transformation, simply specify the current transformation. [2]
				.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque) // => no blending with other windows
				.setPresentMode(get_config_presentation_mode(surface()))
				.setClipped(VK_TRUE) // we don't care about the color of pixels that are obscured, for example because another window is in front of them.  [2]
				.setOldSwapchain({}) // TODO: This won't be enought, I'm afraid/pretty sure. => advanced chapter
				.setImageSharingMode(mImageCreateInfoSwapChain.sharingMode)
				.setQueueFamilyIndexCount(mImageCreateInfoSwapChain.queueFamilyIndexCount)
				.setPQueueFamilyIndices(mImageCreateInfoSwapChain.pQueueFamilyIndices);

			mSwapChainImageFormat = surfaceFormat.format;
		}

		mSwapChainExtent = mSwapChainCreateInfo.imageExtent;
	}

	void window::construct_backbuffers(swapchain_creation_mode aCreationMode) {
		const auto swapChainImages = context().device().getSwapchainImagesKHR(swap_chain());
		const auto imagesInFlight = swapChainImages.size();

		assert(imagesInFlight == get_config_number_of_presentable_images()); // TODO: Can it happen that these two ever differ? If so => handle!

		auto extent = context().get_resolution_for_window(this);
		auto imageResize = [&extent](avk::image_t& aPreparedImage) {
			if (aPreparedImage.depth() == 1u) {
				aPreparedImage.create_info().extent.width = extent.x;
				aPreparedImage.create_info().extent.height = extent.y;
			}
			else {
				LOG_WARNING(std::format("No idea how to update a 3D image with dimensions {}x{}x{}", aPreparedImage.width(), aPreparedImage.height(), aPreparedImage.depth()));
			}
		};
		auto lifetimeHandlerLambda = [this](any_window_resource_t&& rhs) { this->handle_lifetime(std::move(rhs)); };

		// Create the new image views:
		std::vector<avk::image_view> newImageViews;
		newImageViews.reserve(imagesInFlight);
		for (size_t i = 0; i < imagesInFlight; ++i) {
			auto& ref = newImageViews.emplace_back(context().create_image_view(context().wrap_image(swapChainImages[i], mImageCreateInfoSwapChain, get_config_image_usage_properties(), vk::ImageAspectFlagBits::eColor)));
			ref.enable_shared_ownership();
		}

		avk::assign_and_lifetime_handle_previous(mSwapChainImageViews, std::move(newImageViews), lifetimeHandlerLambda);

		bool additionalAttachmentsChanged = mResourceRecreationDeterminator.is_recreation_required_for(recreation_determinator::reason::backbuffer_attachments_changed);
		bool imageFormatChanged = mResourceRecreationDeterminator.is_recreation_required_for(recreation_determinator::reason::image_format_or_properties_changed);

		auto additionalAttachments = get_additional_back_buffer_attachments();
		// Create a renderpass for the back buffers
		std::vector<avk::attachment> renderpassAttachments = {
			avk::attachment::declare_for(*mSwapChainImageViews[0], avk::on_load::clear.from_previous_layout(avk::layout::undefined), avk::usage::color(0), avk::on_store::store.in_layout(avk::layout::color_attachment_optimal))
		};
		renderpassAttachments.insert(std::end(renderpassAttachments), std::begin(additionalAttachments), std::end(additionalAttachments));

		//recreate render pass only if really necessary, otherwise keep using the old one
		if (aCreationMode == swapchain_creation_mode::create_new_swapchain || imageFormatChanged || additionalAttachmentsChanged)
		{
			auto newRenderPass = context().create_renderpass(renderpassAttachments, {
				// We only create one subpass here => create default dependencies as per specification chapter 8.1) Render Pass Creation:
				avk::subpass_dependency{avk::subpass::external >> avk::subpass::index(0),
					stage::late_fragment_tests | stage::color_attachment_output    >>   stage::early_fragment_tests | stage::late_fragment_tests | stage::color_attachment_output,
					access::depth_stencil_attachment_write                         >>   access::color_attachment_read | access::color_attachment_write | access::depth_stencil_attachment_read | access::depth_stencil_attachment_write
				},
				avk::subpass_dependency{avk::subpass::index(0) >> avk::subpass::external,
					stage::color_attachment_output    >>   stage::color_attachment_output,
					access::color_attachment_write    >>   access::none
				}
			});
			if (mBackBufferRenderpass.has_value() && mBackBufferRenderpass.is_shared_ownership_enabled()) {
				newRenderPass.enable_shared_ownership();
			}
			avk::assign_and_lifetime_handle_previous(mBackBufferRenderpass, std::move(newRenderPass), lifetimeHandlerLambda);
		}
		
		std::vector<avk::framebuffer> newBuffers;
		newBuffers.reserve(imagesInFlight);
		for (size_t i = 0; i < imagesInFlight; ++i) {
			auto& imView = mSwapChainImageViews[i];
			auto imExtent = imView->get_image().create_info().extent;
			// Create one image view per attachment
			std::vector<avk::image_view> imageViews;
			imageViews.reserve(renderpassAttachments.size());
			imageViews.push_back(imView); // The color attachment is added in any case
			// reuse image views if updating, however not if there are new additional attachments
			// if the number of presentation images is now higher than the previous creation, we need new image views on top of previous ones
			if (aCreationMode == swapchain_creation_mode::update_existing_swapchain && !additionalAttachmentsChanged && i < mBackBuffers.size()) {
				const auto& backBufferImageViews = mBackBuffers[i]->image_views();
				for (int j = 1; j < backBufferImageViews.size(); j++) {
					imageViews.emplace_back(context().create_image_view_from_template(*backBufferImageViews[j], imageResize));
				}
			}
			else {
				for (auto& aa : additionalAttachments) {
					if (aa.is_used_as_depth_stencil_attachment()) {
						imageViews.emplace_back(context().create_depth_image_view(
							context().create_image(imExtent.width, imExtent.height, aa.format(), 1, avk::memory_usage::device, avk::image_usage::read_only_depth_stencil_attachment))); // TODO: read_only_* or better general_*?
					}
					else {
						imageViews.emplace_back(context().create_image_view(
							context().create_image(imExtent.width, imExtent.height, aa.format(), 1, avk::memory_usage::device, avk::image_usage::general_color_attachment)));
					}
				}
			}
			auto& ref = newBuffers.emplace_back(context().create_framebuffer(mBackBufferRenderpass, std::move(imageViews), extent.x, extent.y));
			ref.enable_shared_ownership();
		}

		avk::assign_and_lifetime_handle_previous(mBackBuffers, std::move(newBuffers), lifetimeHandlerLambda);
	}
}
