#include <gvk.hpp>

namespace gvk
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
			mResourceRecreationDeterminator.set_recreation_required_for(recreation_determinator::reason::image_format_changed);
		}
	}

	void window::set_presentaton_mode(gvk::presentation_mode aMode)
	{
		mPresentationModeSelector = [lPresMode = aMode](const vk::SurfaceKHR& surface) {
			// Supported presentation modes must be queried from a device:
			auto presModes = context().physical_device().getSurfacePresentModesKHR(surface);

			// Select a presentation mode:
			auto selPresModeItr = presModes.end();
			switch (lPresMode) {
			case gvk::presentation_mode::immediate:
				selPresModeItr = std::find(std::begin(presModes), std::end(presModes), vk::PresentModeKHR::eImmediate);
				break;
			case gvk::presentation_mode::relaxed_fifo:
				selPresModeItr = std::find(std::begin(presModes), std::end(presModes), vk::PresentModeKHR::eFifoRelaxed);
				break;
			case gvk::presentation_mode::fifo:
				selPresModeItr = std::find(std::begin(presModes), std::end(presModes), vk::PresentModeKHR::eFifo);
				break;
			case gvk::presentation_mode::mailbox:
				selPresModeItr = std::find(std::begin(presModes), std::end(presModes), vk::PresentModeKHR::eMailbox);
				break;
			default:
				throw gvk::runtime_error("should not get here");
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
				throw gvk::runtime_error("Failed to create window with the title '" + mTitle + "'");
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
			set_presentaton_mode(gvk::presentation_mode::mailbox);
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

	void window::handle_lifetime(avk::resource_ownership<avk::command_buffer_t> aCommandBuffer, std::optional<frame_id_t> aFrameId)
	{
		std::scoped_lock<std::mutex> guard(sSubmitMutex); // Protect against concurrent access from invokees
		if (!aFrameId.has_value()) {
			aFrameId = current_frame();
		}

		aCommandBuffer->invoke_post_execution_handler(); // Yes, do it now!

		auto& refTpl = mLifetimeHandledCommandBuffers.emplace_back(aFrameId.value(), aCommandBuffer.own());
		// ^ Prefer code duplication over recursive_mutex
	}

	void window::handle_lifetime(outdated_swapchain_resource_t&& aOutdatedSwapchain, std::optional<frame_id_t> aFrameId)
	{
		std::scoped_lock<std::mutex> guard(sSubmitMutex); // Protect against concurrent access from invokees
		if (!aFrameId.has_value()) {
			aFrameId = current_frame();
		}
		mOutdatedSwapChainResources.emplace_back(aFrameId.value(), std::move(aOutdatedSwapchain));
	}

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

	std::vector<avk::command_buffer> window::clean_up_command_buffers_for_frame(frame_id_t aPresentFrameId)
	{
		std::vector<avk::command_buffer> removedBuffers;
		if (mLifetimeHandledCommandBuffers.empty()) {
			return removedBuffers;
		}
		// No need to protect against concurrent access since that would be misuse of this function.
		// This shall never be called from the invokee callbacks as being invoked through a parallel invoker.

		// Up to the frame with id 'maxTTL', all command buffers can be safely removed
		const auto maxTTL = aPresentFrameId - number_of_frames_in_flight();

		// 1. SINGLE USE COMMAND BUFFERS
		// Can not use the erase-remove idiom here because that would invalidate iterators and references
		// HOWEVER: "[...]unless the erased elements are at the end or the beginning of the container,
		// in which case only the iterators and references to the erased elements are invalidated." => Let's do that!
		auto eraseBegin = std::begin(mLifetimeHandledCommandBuffers);
		if (std::end(mLifetimeHandledCommandBuffers) == eraseBegin || std::get<frame_id_t>(*eraseBegin) > maxTTL) {
			return removedBuffers;
		}
		// There are elements that we can remove => find position until where:
		auto eraseEnd = eraseBegin;
		while (eraseEnd != std::end(mLifetimeHandledCommandBuffers) && std::get<frame_id_t>(*eraseEnd) <= maxTTL) {
			// return ownership of all the command_buffers to remove to the caller
			removedBuffers.push_back(std::move(std::get<avk::command_buffer>(*eraseEnd)));
			++eraseEnd;
		}
		mLifetimeHandledCommandBuffers.erase(eraseBegin, eraseEnd);
		return removedBuffers;
	}

	void window::clean_up_outdated_swapchain_resources_for_frame(frame_id_t aPresentFrameId)
	{
		if (mOutdatedSwapChainResources.empty()) {
			return;
		}

		// Up to the frame with id 'maxTTL', all swap chain resources can be safely removed
		const auto maxTTL = aPresentFrameId - number_of_frames_in_flight();

		auto eraseBegin = std::begin(mOutdatedSwapChainResources);
		if (std::end(mOutdatedSwapChainResources) == eraseBegin || std::get<frame_id_t>(*eraseBegin) > maxTTL) {
			return;
		}
		auto eraseEnd = eraseBegin;
		while (eraseEnd != std::end(mOutdatedSwapChainResources) && std::get<frame_id_t>(*eraseEnd) <= maxTTL) {
			++eraseEnd;
		}
		mOutdatedSwapChainResources.erase(eraseBegin, eraseEnd);
	}

	void window::fill_in_present_semaphore_dependencies_for_frame(std::vector<vk::Semaphore>& aSemaphores, std::vector<vk::PipelineStageFlags>& aWaitStages, frame_id_t aFrameId) const
	{
		for (const auto& [frameId, sem] : mPresentSemaphoreDependencies) {
			if (frameId == aFrameId) {
				aSemaphores.push_back(sem->handle());
				aWaitStages.push_back(sem->semaphore_wait_stage());
			}
		}
	}

	void window::acquire_next_swap_chain_image_and_prepare_semaphores()
	{
		if (mResourceRecreationDeterminator.is_any_recreation_necessary())	{
			if (mResourceRecreationDeterminator.has_concurrent_frames_count_changed_only())	{ //only update framesInFlight fences/semaphores
				mPresentQueue->handle().waitIdle(); // ensure the semaphors which we are going to potentially delete are not being used
				update_concurrent_frame_synchronization(swapchain_creation_mode::update_existing_swapchain);
				mResourceRecreationDeterminator.reset();
			}
			else { //recreate the whole swap chain then
				update_resolution_and_recreate_swap_chain();
			}
		}

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
				mPresentQueue->handle().submit({ // TODO: This works, but is arguably not the greatest of all solutions... => Can it be done better with Timeline Semaphores? (Test on AMD!)
					vk::SubmitInfo{}
						.setCommandBufferCount(0u)
						.setWaitSemaphoreCount(1u)
						.setPWaitSemaphores(imgAvailableSem->handle_addr())
						.setPWaitDstStageMask(imgAvailableSem->semaphore_wait_stage_addr())
				}, fen->handle());
				fen->wait_until_signalled();

				acquire_next_swap_chain_image_and_prepare_semaphores();
				return;
			}
		}
		catch (vk::OutOfDateKHRError omg) {
			LOG_INFO(fmt::format("Swap chain out of date in acquire_next_swap_chain_image_and_prepare_semaphores. Reason[{}] in frame#{}. Going to recreate it...", omg.what(), current_frame()));
			mResourceRecreationDeterminator.set_recreation_required_for(recreation_determinator::reason::invalid_swap_chain);
			acquire_next_swap_chain_image_and_prepare_semaphores();
			return;
		}

		// It could be that the image index that has been returned is currently in flight.
		// There's no guarantee that we'll always get a nice cycling through the indices.
		// => Must handle this case!
		assert(current_image_index() == mCurrentFrameImageIndex);
		if (mImagesInFlightFenceIndices[current_image_index()] >= 0) {
			LOG_DEBUG_VERBOSE(fmt::format("Frame #{}: Have to issue an extra fence-wait because swap chain returned image[{}] but fence[{}] is currently in use.", current_frame(), mCurrentFrameImageIndex, mImagesInFlightFenceIndices[current_image_index()]));
			auto& xf = mFramesInFlightFences[mImagesInFlightFenceIndices[current_image_index()]];
			xf->wait_until_signalled();
			// But do not reset! Otherwise we will wait forever at the next wait_until_signalled that will happen for sure.
		}

		// Set the image available semaphore to be consumed:
		mCurrentFrameImageAvailableSemaphore = std::ref(imgAvailableSem);
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
		auto commandBuffersToBeFreed 	= clean_up_command_buffers_for_frame(current_frame());
		clean_up_outdated_swapchain_resources_for_frame(current_frame());

		acquire_next_swap_chain_image_and_prepare_semaphores();
	}

	void window::render_frame()
	{
		const auto fenceIndex = static_cast<int>(current_in_flight_index());
		const auto& cf = *mFramesInFlightFences[fenceIndex];

		// EXTERN -> WAIT
		std::vector<vk::Semaphore> renderFinishedSemaphores;
		std::vector<vk::PipelineStageFlags> renderFinishedSemaphoreStages;
		fill_in_present_semaphore_dependencies_for_frame(renderFinishedSemaphores, renderFinishedSemaphoreStages, current_frame());

		if (!has_consumed_current_image_available_semaphore()) {
			// Being in this branch indicates that image available semaphore has not been consumed yet
			// meaning that no render calls were (correctly) executed  (hint: check if all invokess are possibly disabled).
			auto imgAvailable = consume_current_image_available_semaphore();
			renderFinishedSemaphores.push_back(imgAvailable->handle());
			renderFinishedSemaphoreStages.push_back(imgAvailable->semaphore_wait_stage());
		}

		// TODO: What if the user has not submitted any renderFinishedSemaphores?

		// WAIT -> SIGNAL
		auto signalSemaphore = current_initiate_present_semaphore();

		auto submitInfo = vk::SubmitInfo()
			.setWaitSemaphoreCount(static_cast<uint32_t>(renderFinishedSemaphores.size()))
			.setPWaitSemaphores(renderFinishedSemaphores.data())
			.setPWaitDstStageMask(renderFinishedSemaphoreStages.data())
			.setCommandBufferCount(0u) // Submit ZERO command buffers :O
			.setSignalSemaphoreCount(1u)
			.setPSignalSemaphores(signalSemaphore->handle_addr());
		// SIGNAL + FENCE, actually:
		assert(mPresentQueue);
		mPresentQueue->handle().submit(1u, &submitInfo, cf.handle());

		try
		{
			// SIGNAL -> PRESENT
			auto presentInfo = vk::PresentInfoKHR()
				.setWaitSemaphoreCount(1u)
				.setPWaitSemaphores(signalSemaphore->handle_addr())
				.setSwapchainCount(1u)
				.setPSwapchains(&swap_chain())
				.setPImageIndices(&mCurrentFrameImageIndex)
				.setPResults(nullptr);
			auto result = mPresentQueue->handle().presentKHR(presentInfo);

			// Submitted => store the image index for extra reuse-safety:
			mImagesInFlightFenceIndices[mCurrentFrameImageIndex] = fenceIndex;

			if (vk::Result::eSuboptimalKHR == result) {
				LOG_INFO("Swap chain is suboptimal in render_frame. Going to recreate it...");
				mResourceRecreationDeterminator.set_recreation_required_for(recreation_determinator::reason::suboptimal_swap_chain);
				// swap chain will be recreated in the next frame
			}
		}
		catch (vk::OutOfDateKHRError omg) {
			LOG_INFO(fmt::format("Swap chain out of date in render_frame. Reason[{}] in frame#{}. Going to recreate it...", omg.what(), current_frame()));
			mResourceRecreationDeterminator.set_recreation_required_for(recreation_determinator::reason::invalid_swap_chain);
			// Just do nothing. Ignore the failure. This frame is lost.
			// swap chain will be recreated in the next frame
		}

		// increment frame counter
		++mCurrentFrame;
	}

	void window::add_queue_family_ownership(avk::queue& aQueue)
	{
		mQueueFamilyIndicesGetter.emplace_back([pQueue = &aQueue](){ return pQueue->family_index(); });
	}

	void window::set_present_queue(avk::queue& aPresentQueue)
	{
		mPresentQueue = &aPresentQueue;
	}

	void window::create_swap_chain(swapchain_creation_mode aCreationMode)
	{
		if (aCreationMode == window::swapchain_creation_mode::create_new_swapchain) {
			mCurrentFrame = 0; // Start af frame 0
		}

		construct_swap_chain_creation_info(aCreationMode);

		auto lifetimeHandler = [this](vk::UniqueSwapchainKHR&& aOldResource) { this->handle_lifetime(std::move(aOldResource)); };
		// assign the new swap chain instead of the old one, if one exists
		avk::assign_and_lifetime_handle_previous(mSwapChain, context().device().createSwapchainKHRUnique(mSwapChainCreateInfo), lifetimeHandler);

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
		mInitiatePresentSemaphores.clear();
		mFramesInFlightFences.reserve(framesInFlight);
		mImageAvailableSemaphores.reserve(framesInFlight);
		mInitiatePresentSemaphores.reserve(framesInFlight);
		for (uint32_t i = 0; i < framesInFlight; ++i) {
			mFramesInFlightFences.push_back(context().create_fence(true)); // true => Create the fences in signalled state, so that `cgb::context().logical_device().waitForFences` at the beginning of `window::render_frame` is not blocking forever, but can continue immediately.
			mImageAvailableSemaphores.push_back(context().create_semaphore());
			mInitiatePresentSemaphores.push_back(context().create_semaphore());
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
		update_resolution();
		std::atomic_bool resolutionUpdated = false;
		context().dispatch_to_main_thread([&resolutionUpdated]() { resolutionUpdated = true; });
		context().signal_waiting_main_thread();
		mPresentQueue->handle().waitIdle();
		while(!resolutionUpdated) { LOG_DEBUG("Waiting for main thread..."); }

		create_swap_chain(swapchain_creation_mode::update_existing_swapchain);
	}

	void window::construct_swap_chain_creation_info(swapchain_creation_mode aCreationMode) {
		auto srfCaps = context().physical_device().getSurfaceCapabilitiesKHR(surface());
		auto extent = context().get_resolution_for_window(this);
		auto surfaceFormat = get_config_surface_format(surface());
		if (aCreationMode == swapchain_creation_mode::update_existing_swapchain) {
			mImageCreateInfoSwapChain.setExtent(vk::Extent3D(extent.x, extent.y));
			if (mResourceRecreationDeterminator.is_recreation_required_for(recreation_determinator::reason::image_format_changed)) {
				mImageCreateInfoSwapChain.setFormat(surfaceFormat.format);
			}
		}
		else {
			mImageUsage = avk::image_usage::color_attachment | avk::image_usage::transfer_destination | avk::image_usage::presentable;
			const vk::ImageUsageFlags swapChainImageUsageVk = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
			mImageCreateInfoSwapChain = vk::ImageCreateInfo{}
				.setImageType(vk::ImageType::e2D)
				.setFormat(surfaceFormat.format)
				.setExtent(vk::Extent3D(extent.x, extent.y))
				.setMipLevels(1)
				.setArrayLayers(1)
				.setSamples(vk::SampleCountFlagBits::e1)
				.setTiling(vk::ImageTiling::eOptimal)
				.setUsage(swapChainImageUsageVk)
				.setInitialLayout(vk::ImageLayout::eUndefined);

			// Handle queue family ownership:

			std::vector<uint32_t> queueFamilyIndices;
			for (auto& getter : mQueueFamilyIndicesGetter) {
				auto familyIndex = getter();
				if (std::end(queueFamilyIndices) == std::find(std::begin(queueFamilyIndices), std::end(queueFamilyIndices), familyIndex)) {
					queueFamilyIndices.push_back(familyIndex);
				}
			}

			switch (queueFamilyIndices.size()) {
			case 0:
				throw gvk::runtime_error(fmt::format("You must assign at least set one queue(family) to window '{}'! You can use add_queue_family_ownership().", title()));
			case 1:
				mImageCreateInfoSwapChain
					.setSharingMode(vk::SharingMode::eExclusive)
					.setQueueFamilyIndexCount(0u)
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
			if (mResourceRecreationDeterminator.is_recreation_required_for(recreation_determinator::reason::image_format_changed)) {
				mSwapChainCreateInfo.setImageColorSpace(surfaceFormat.colorSpace);
				mSwapChainCreateInfo.setImageFormat(mImageCreateInfoSwapChain.format);
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
				LOG_WARNING(fmt::format("No idea how to update a 3D image with dimensions {}x{}x{}", aPreparedImage.width(), aPreparedImage.height(), aPreparedImage.depth()));
			}
		};
		auto lifetimeHandlerLambda = [this](outdated_swapchain_resource_t&& rhs) { this->handle_lifetime(std::move(rhs)); };

		// Create the new image views:
		std::vector<avk::image_view> newImageViews;
		newImageViews.reserve(imagesInFlight);
		for (size_t i = 0; i < imagesInFlight; ++i) {
			auto& ref = newImageViews.emplace_back(context().create_image_view(context().wrap_image(swapChainImages[i], mImageCreateInfoSwapChain, mImageUsage, vk::ImageAspectFlagBits::eColor)));
			ref.enable_shared_ownership();
		}

		avk::assign_and_lifetime_handle_previous(mSwapChainImageViews, std::move(newImageViews), lifetimeHandlerLambda);

		bool additionalAttachmentsChanged = mResourceRecreationDeterminator.is_recreation_required_for(recreation_determinator::reason::backbuffer_attachments_changed);
		bool imageFormatChanged = mResourceRecreationDeterminator.is_recreation_required_for(recreation_determinator::reason::image_format_changed);

		auto additionalAttachments = get_additional_back_buffer_attachments();
		// Create a renderpass for the back buffers
		std::vector<avk::attachment> renderpassAttachments = {
			avk::attachment::declare_for(const_referenced(mSwapChainImageViews[0]), avk::on_load::clear, avk::color(0), avk::on_store::store)
		};
		renderpassAttachments.insert(std::end(renderpassAttachments), std::begin(additionalAttachments), std::end(additionalAttachments));

		//recreate render pass only if really necessary, otherwise keep using the old one
		if (aCreationMode == swapchain_creation_mode::create_new_swapchain || imageFormatChanged || additionalAttachmentsChanged)
		{
			auto newRenderPass = context().create_renderpass(renderpassAttachments);
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
			std::vector<avk::resource_ownership<avk::image_view_t>> imageViews;
			imageViews.reserve(renderpassAttachments.size());
			imageViews.push_back(avk::shared(imView)); // The color attachment is added in any case
			// reuse image views if updating, however not if there are new additional attachments
			// if the number of presentation images is now higher than the previous creation, we need new image views on top of previous ones
			if (aCreationMode == swapchain_creation_mode::update_existing_swapchain && !additionalAttachmentsChanged && i < mBackBuffers.size()) {
				const auto& backBufferImageViews = mBackBuffers[i]->image_views();
				for (int j = 1; j < backBufferImageViews.size(); j++) {
					imageViews.emplace_back(gvk::context().create_image_view_from_template(backBufferImageViews[j], imageResize));
				}
			}
			else {
				for (auto& aa : additionalAttachments) {
					if (aa.is_used_as_depth_stencil_attachment()) {
						imageViews.emplace_back(context().create_depth_image_view(
							avk::owned(context().create_image(imExtent.width, imExtent.height, aa.format(), 1, avk::memory_usage::device, avk::image_usage::read_only_depth_stencil_attachment)))); // TODO: read_only_* or better general_*?
					}
					else {
						imageViews.emplace_back(context().create_image_view(
							avk::owned(context().create_image(imExtent.width, imExtent.height, aa.format(), 1, avk::memory_usage::device, avk::image_usage::general_color_attachment))));
					}
				}
			}
			auto& ref = newBuffers.emplace_back(gvk::context().create_framebuffer(avk::shared(mBackBufferRenderpass), std::move(imageViews), extent.x, extent.y));
			ref.enable_shared_ownership();
		}

		avk::assign_and_lifetime_handle_previous(mBackBuffers, std::move(newBuffers), lifetimeHandlerLambda);

		// Transfer the backbuffer images into a at least somewhat useful layout for a start:
		for (auto& bb : mBackBuffers) {
			const auto n = bb->image_views().size();
			assert(n == get_renderpass()->number_of_attachment_descriptions());
			for (size_t i = 0; i < n; ++i) {
				bb->image_view_at(i)->get_image().transition_to_layout(get_renderpass()->attachment_descriptions()[i].finalLayout, avk::sync::wait_idle(true));
			}
		}
	}
}
