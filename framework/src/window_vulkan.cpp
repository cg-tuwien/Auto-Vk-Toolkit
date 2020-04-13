#include <cg_base.hpp>

namespace cgb
{
	std::mutex window::sSubmitMutex;
	
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
						if (is_srgb_format(cgb::image_format(e))) {
							selSurfaceFormat = e;
							break;
						}
					}
					else {
						if (!is_srgb_format(cgb::image_format(e))) {
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
			mRecreationRequired = true;
		}
	}

	void window::set_presentaton_mode(cgb::presentation_mode aMode)
	{
		mPresentationModeSelector = [lPresMode = aMode](const vk::SurfaceKHR& surface) {
			// Supported presentation modes must be queried from a device:
			auto presModes = context().physical_device().getSurfacePresentModesKHR(surface);

			// Select a presentation mode:
			auto selPresModeItr = presModes.end();
			switch (lPresMode) {
			case cgb::presentation_mode::immediate:
				selPresModeItr = std::find(std::begin(presModes), std::end(presModes), vk::PresentModeKHR::eImmediate);
				break;
			case cgb::presentation_mode::relaxed_fifo:
				selPresModeItr = std::find(std::begin(presModes), std::end(presModes), vk::PresentModeKHR::eFifoRelaxed);
				break;
			case cgb::presentation_mode::fifo:
				selPresModeItr = std::find(std::begin(presModes), std::end(presModes), vk::PresentModeKHR::eFifo);
				break;
			case cgb::presentation_mode::mailbox:
				selPresModeItr = std::find(std::begin(presModes), std::end(presModes), vk::PresentModeKHR::eMailbox);
				break;
			default:
				throw cgb::runtime_error("should not get here");
			}
			if (selPresModeItr == presModes.end()) {
				LOG_WARNING_EM("No presentation mode specified or desired presentation mode not available => will select any presentation mode");
				selPresModeItr = presModes.begin();
			}

			return *selPresModeItr;
		};

		// If the window has already been created, the new setting can't 
		// be applied unless the window is being recreated.
		if (is_alive()) {
			mRecreationRequired = true;
		}
	}

	void window::set_number_of_samples(int aNumSamples)
	{
		mNumberOfSamplesGetter = [lSamples = to_vk_sample_count(aNumSamples)]() { return lSamples; };

		mMultisampleCreateInfoBuilder = [this]() {
			auto samples = mNumberOfSamplesGetter();
			return vk::PipelineMultisampleStateCreateInfo()
				.setSampleShadingEnable(vk::SampleCountFlagBits::e1 == samples ? VK_FALSE : VK_TRUE) // disable/enable?
				.setRasterizationSamples(samples)
				.setMinSampleShading(1.0f) // Optional
				.setPSampleMask(nullptr) // Optional
				.setAlphaToCoverageEnable(VK_FALSE) // Optional
				.setAlphaToOneEnable(VK_FALSE); // Optional
		};

		// If the window has already been created, the new setting can't 
		// be applied unless the window is being recreated.
		if (is_alive()) {
			mRecreationRequired = true;
		}
	}

	void window::set_number_of_presentable_images(uint32_t aNumImages)
	{
		mNumberOfPresentableImagesGetter = [lNumImages = aNumImages]() { return lNumImages; };

		// If the window has already been created, the new setting can't 
		// be applied unless the window is being recreated.
		if (is_alive()) {
			mRecreationRequired = true;
		}
	}

	void window::set_number_of_concurrent_frames(uint32_t aNumConcurrent)
	{
		mNumberOfConcurrentFramesGetter = [lNumConcurrent = aNumConcurrent]() { return lNumConcurrent; };

		// If the window has already been created, the new setting can't 
		// be applied unless the window is being recreated.
		if (is_alive()) {
			mRecreationRequired = true;
		}
	}

	void window::set_additional_back_buffer_attachments(std::vector<attachment> aAdditionalAttachments)
	{
		mAdditionalBackBufferAttachmentsGetter = [lAdditionalAttachments = std::move(aAdditionalAttachments)]() { return lAdditionalAttachments; };

		// If the window has already been created, the new setting can't 
		// be applied unless the window is being recreated.
		if (is_alive()) {
			mRecreationRequired = true;
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
			auto* handle = glfwCreateWindow(mRequestedSize.mWidth, mRequestedSize.mHeight,
				mTitle.c_str(),
				mMonitor.has_value() ? mMonitor->mHandle : nullptr,
				sharedContex);
			if (nullptr == handle) {
				// No point in continuing
				throw new cgb::runtime_error("Failed to create window with the title '" + mTitle + "'");
			}
			mHandle = window_handle{ handle };
			initialize_after_open();

			// There will be some pending work regarding this newly created window stored within the
			// context's events, like creating a swap chain and so on. 
			// Why wait? Invoke them now!
			context().work_off_event_handlers();
		});
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
			set_presentaton_mode(cgb::presentation_mode::mailbox);
		}
		// Determine the presentation mode:
		return mPresentationModeSelector(aSurface);
	}

	vk::SampleCountFlagBits window::get_config_number_of_samples()
	{
		if (!mNumberOfSamplesGetter) {
			// Set the default:
			set_number_of_samples(1);
		}
		// Determine the number of samples:
		return mNumberOfSamplesGetter();
	}

	vk::PipelineMultisampleStateCreateInfo window::get_config_multisample_state_create_info()
	{
		if (!mMultisampleCreateInfoBuilder) {
			// Set the default:
			set_number_of_samples(1);
		}
		// Get the config struct:
		return mMultisampleCreateInfoBuilder();
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

	uint32_t window::get_config_number_of_concurrent_frames()
	{
		if (!mNumberOfConcurrentFramesGetter) {
			return get_config_number_of_presentable_images();
		}
		return mNumberOfConcurrentFramesGetter();
	}

	std::vector<attachment> window::get_additional_back_buffer_attachments()
	{
		if (!mAdditionalBackBufferAttachmentsGetter) {
			return {};
		}
		else {
			return mAdditionalBackBufferAttachmentsGetter();
		}
	}

	void window::set_extra_semaphore_dependency(semaphore aSemaphore, std::optional<int64_t> aFrameId)
	{
		if (!aFrameId.has_value()) {
			aFrameId = current_frame();
		}
		mExtraSemaphoreDependencies.emplace_back(aFrameId.value(), std::move(aSemaphore));
	}

	void window::handle_single_use_command_buffer_lifetime(command_buffer aCommandBuffer, std::optional<int64_t> aFrameId)
	{
		std::scoped_lock<std::mutex> guard(sSubmitMutex);
		if (!aFrameId.has_value()) {
			aFrameId = current_frame();
		}
		mSingleUseCommandBuffers.emplace_back(aFrameId.value(), std::move(aCommandBuffer));
	}

	void window::submit_for_backbuffer(command_buffer aCommandBuffer, std::optional<int64_t> aFrameId)
	{
		std::scoped_lock<std::mutex> guard(sSubmitMutex);
		if (!aFrameId.has_value()) {
			aFrameId = current_frame();
		}

		aCommandBuffer->invoke_post_execution_handler(); // Yes, do it now!
		
		const auto frameId = aFrameId.value();
		auto& refTpl = mSingleUseCommandBuffers.emplace_back(frameId, std::move(aCommandBuffer));
		mCommandBufferRefs.emplace_back(frameId, std::cref(*std::get<command_buffer>(refTpl)));
		// ^ Prefer code duplication over recursive_mutex
	}

	void window::submit_for_backbuffer(std::optional<command_buffer> aCommandBuffer, std::optional<int64_t> aFrameId)
	{
		if (!aCommandBuffer.has_value()) {
			LOG_WARNING("std::optional<command_buffer> submitted and it has no value.");
			return;
		}
		submit_for_backbuffer(std::move(aCommandBuffer.value()), aFrameId);
	}

	void window::submit_for_backbuffer_ref(const command_buffer_t& aCommandBuffer, std::optional<int64_t> aFrameId)
	{
		std::scoped_lock<std::mutex> guard(sSubmitMutex);
		if (!aFrameId.has_value()) {
			aFrameId = current_frame();
		}

		aCommandBuffer.invoke_post_execution_handler(); // Yes, do it now!
		
		mCommandBufferRefs.emplace_back(aFrameId.value(), std::cref(aCommandBuffer));
	}

	std::vector<semaphore> window::remove_all_extra_semaphore_dependencies_for_frame(int64_t aPresentFrameId)
	{
		// No need to protect against concurrent access since that would be misuse of this function.
		// This shall never be called from the cg_element callbacks as being invoked through a parallel executor.
		
		// Find all to remove
		auto to_remove = std::remove_if(
			std::begin(mExtraSemaphoreDependencies), std::end(mExtraSemaphoreDependencies),
			[maxTTL = std::min(aPresentFrameId - number_of_in_flight_frames(), aPresentFrameId - number_of_swapchain_images())](const auto& tpl) {
				return std::get<int64_t>(tpl) <= maxTTL;
			});
		// return ownership of all the semaphores to remove to the caller
		std::vector<semaphore> moved_semaphores;
		for (decltype(to_remove) it = to_remove; it != std::end(mExtraSemaphoreDependencies); ++it) {
			moved_semaphores.push_back(std::move(std::get<semaphore>(*it)));
		}
		// Erase and return
		mExtraSemaphoreDependencies.erase(to_remove, std::end(mExtraSemaphoreDependencies));
		return moved_semaphores;
	}

	std::vector<command_buffer> window::clean_up_command_buffers_for_frame(int64_t aPresentFrameId)
	{
		// No need to protect against concurrent access since that would be misuse of this function.
		// This shall never be called from the cg_element callbacks as being invoked through a parallel executor.

		// Up to the frame with id 'maxTTL', all command buffers can be safely removed
		auto maxTTL = std::min(aPresentFrameId - number_of_in_flight_frames(), aPresentFrameId - number_of_swapchain_images());
		
		// 1. COMMAND BUFFER REFERENCES
		const auto refs_to_remove = std::remove_if(
			std::begin(mCommandBufferRefs), std::end(mCommandBufferRefs),
			[maxTTL](const auto& tpl){
				return std::get<int64_t>(tpl) <= maxTTL;
			});
		mCommandBufferRefs.erase(refs_to_remove, std::end(mCommandBufferRefs));
		
		// 2. SINGLE USE COMMAND BUFFERS
		// Can not use the erase-remove idiom here because that would invalidate iterators and references
		// HOWEVER: "[...]unless the erased elements are at the end or the beginning of the container,
		// in which case only the iterators and references to the erased elements are invalidated." => Let's do that!
		auto eraseBegin = std::begin(mSingleUseCommandBuffers);
		std::vector<command_buffer> removedBuffers;
		if (std::end(mSingleUseCommandBuffers) == eraseBegin || std::get<int64_t>(*eraseBegin) > maxTTL) {
			return removedBuffers;
		}
		// There are elements that we can remove => find position until where:
		auto eraseEnd = eraseBegin;
		while (eraseEnd != std::end(mSingleUseCommandBuffers) && std::get<int64_t>(*eraseEnd) <= maxTTL) {
			// return ownership of all the command_buffers to remove to the caller
			removedBuffers.push_back(std::move(std::get<command_buffer>(*eraseEnd)));
			++eraseEnd;
		}
		mSingleUseCommandBuffers.erase(eraseBegin, eraseEnd);
		return removedBuffers;
	}

	std::vector<std::reference_wrapper<const cgb::command_buffer_t>> window::get_command_buffer_refs_for_frame(int64_t aFrameId) const
	{
		std::vector<std::reference_wrapper<const cgb::command_buffer_t>> result;
		for (const auto& [cbFrameId, ref] : mCommandBufferRefs) {
			if (cbFrameId == aFrameId) {
				result.push_back(ref);
			}
		}
		return result;
	}

	void window::fill_in_extra_semaphore_dependencies_for_frame(std::vector<vk::Semaphore>& aSemaphores, int64_t aFrameId) const
	{
		for (const auto& [frameId, sem] : mExtraSemaphoreDependencies) {
			if (frameId == aFrameId) {
				aSemaphores.push_back(sem->handle());
			}
		}
	}

	void window::fill_in_extra_render_finished_semaphores_for_frame(std::vector<vk::Semaphore>& aSemaphores, int64_t aFrameId) const
	{
		// TODO: Fill mExtraRenderFinishedSemaphores with meaningful data
		// TODO: Implement
		//auto si = in_flight_index_for_frame();
		//for (auto i = si; i < si + mNumExtraRenderFinishedSemaphoresPerFrame; ++i) {
		//	pSemaphores.push_back(mExtraRenderFinishedSemaphores. [i]->handle());
		//}
	}

	/*std::vector<semaphore> window::set_num_extra_semaphores_to_generate_per_frame(uint32_t _NumExtraSemaphores)
	{

	}*/

	void window::sync_before_render()
	{
		vk::Result result;
		
		// Wait for the fence before proceeding, GPU -> CPU synchronization via fence
		const auto& fence = fence_for_frame();
		cgb::context().logical_device().waitForFences(1u, fence.handle_addr(), VK_TRUE, std::numeric_limits<uint64_t>::max());
		result = cgb::context().logical_device().resetFences(1u, fence.handle_addr());
		assert (vk::Result::eSuccess == result);

		// At this point we are certain that frame which used the current fence before is done.
		//  => Clean up the resources of that previous frame!
		auto semaphoresToBeFreed = remove_all_extra_semaphore_dependencies_for_frame(current_frame());
		auto commandBuffersToBeFreed 	= clean_up_command_buffers_for_frame(current_frame());

		//
		//
		//
		//	TODO: Recreate swap chain probably somewhere here
		//  Potential problems:
		//	 - How to handle the fences? Is waitIdle enough?
		//	 - A problem might be the multithreaded access to this function... hmm... or is it??
		//      => Now would be the perfect time to think about how to handle parallel executors
		//		   Only Command Buffer generation should be parallelized anyways, submission should 
		//		   be done on ONE thread, hence access to this method would be syncronized inherently, right?!
		//
		//	What about the following: Tie an instance of cg_element to ONE AND EXACTLY ONE window*?!
		//	 => Then, the render method would create a command_buffer, which is then gathered (per window!) and passed on to this method.
		//
		//
		//

	}
	
	void window::render_frame()
	{
		vk::Result result;

		const auto& fence = fence_for_frame();

		// Get the next image from the swap chain, GPU -> GPU sync from previous present to the following acquire
		uint32_t imageIndex;
		const auto& imgAvailableSem = image_available_semaphore_for_frame();
		cgb::context().logical_device().acquireNextImageKHR(
			swap_chain(), // the swap chain from which we wish to acquire an image 
			// At this point, I have to rant about the `timeout` parameter:
			// The spec says: "timeout specifies how long the function waits, in nanoseconds, if no image is available."
			// HOWEVER, don't think that a numeric_limit<int64_t>::max() will wait for nine quintillion nanoseconds!
			//    No, instead it will return instantly, yielding an invalid swap chain image index. OMG, WTF?!
			// Long story short: make sure to pass the UNSINGEDint64_t's maximum value, since only that will disable the timeout.
			std::numeric_limits<uint64_t>::max(), // a timeout in nanoseconds for an image to become available. Using the maximum value of a 64 bit unsigned integer disables the timeout. [1]
			imgAvailableSem.handle(), // The next two parameters specify synchronization objects that are to be signaled when the presentation engine is finished using the image [1]
			nullptr,
			&imageIndex); // a variable to output the index of the swap chain image that has become available. The index refers to the VkImage in our swapChainImages array. We're going to use that index to pick the right command buffer. [1]

		auto cbRefs = get_command_buffer_refs_for_frame(current_frame());
		std::vector<vk::CommandBuffer> cmdBuffers;
		cmdBuffers.reserve(cbRefs.size());
		for (auto cb : cbRefs) {
			cmdBuffers.push_back(cb.get().handle());
		}

		// ...and submit them. But also assemble several GPU -> GPU sync objects for both, inbound and outbound sync:
		// Wait for some extra semaphores, if there are any; i.e. GPU -> GPU sync from acquire to the following submit
		std::vector<vk::Semaphore> waitBeforeExecuteSemaphores = { imgAvailableSem.handle() };
		fill_in_extra_semaphore_dependencies_for_frame(waitBeforeExecuteSemaphores, current_frame());
		// For every semaphore, also add a entry for the corresponding stage:
		std::vector<vk::PipelineStageFlags> waitBeforeExecuteStages;
		std::transform( std::begin(waitBeforeExecuteSemaphores), std::end(waitBeforeExecuteSemaphores),
						std::back_inserter(waitBeforeExecuteStages),
						[](const auto & s) { return vk::PipelineStageFlagBits::eColorAttachmentOutput; });
		// Signal at least one semaphore when done, potentially also more.
		const auto& renderFinishedSem = render_finished_semaphore_for_frame();
		std::vector<vk::Semaphore> toSignalAfterExecute = { renderFinishedSem->handle() };
		fill_in_extra_render_finished_semaphores_for_frame(toSignalAfterExecute, current_frame());
		auto submitInfo = vk::SubmitInfo()
			.setWaitSemaphoreCount(static_cast<uint32_t>(waitBeforeExecuteSemaphores.size()))
			.setPWaitSemaphores(waitBeforeExecuteSemaphores.data())
			.setPWaitDstStageMask(waitBeforeExecuteStages.data())
			.setCommandBufferCount(static_cast<uint32_t>(cmdBuffers.size()))
			.setPCommandBuffers(cmdBuffers.data())
			.setSignalSemaphoreCount(static_cast<uint32_t>(toSignalAfterExecute.size()))
			.setPSignalSemaphores(toSignalAfterExecute.data());
		// Finally, submit to the graphics queue.
		// Also provide a fence for GPU -> CPU sync which will be waited on next time we need this frame (top of this method).
		result = cgb::context().graphics_queue().handle().submit(1u, &submitInfo, fence.handle());
		assert (vk::Result::eSuccess == result);
		// Attention: Do not invoke post execution handlers here, they have already been invoked in submit_for_backbuffer/submit_for_backbuffer_ref

		// Present as soon as the render finished semaphore has been signalled:
		auto presentInfo = vk::PresentInfoKHR()
			.setWaitSemaphoreCount(1u)
			.setPWaitSemaphores(&renderFinishedSem->handle())
			.setSwapchainCount(1u)
			.setPSwapchains(&swap_chain())
			.setPImageIndices(&imageIndex)
			.setPResults(nullptr);
		result = cgb::context().presentation_queue().handle().presentKHR(presentInfo);
		assert (vk::Result::eSuccess == result);

		// increment frame counter
		++mCurrentFrame;
	}
	
	std::optional<command_buffer> window::copy_to_swapchain_image(cgb::image_t& aSourceImage, std::optional<int64_t> aDestinationFrameId, bool aShallBePresentedDirectlyAfterwards)
	{
		auto& destinationImage = image_for_frame(aDestinationFrameId);
		return copy_image_to_another(aSourceImage, destinationImage, cgb::sync::with_barriers_by_return(
				cgb::sync::presets::image_copy::wait_for_previous_operations(aSourceImage, destinationImage),
				aShallBePresentedDirectlyAfterwards 
					? cgb::sync::presets::image_copy::directly_into_present(aSourceImage, destinationImage)
					: cgb::sync::presets::image_copy::let_subsequent_operations_wait(aSourceImage, destinationImage)
			),
			true, // Restore layout of source image
			false // Don't restore layout of destination => this is handled by the after-handler in any case
		);
	}
	
	std::optional<command_buffer> window::blit_to_swapchain_image(cgb::image_t& aSourceImage, std::optional<int64_t> aDestinationFrameId, bool aShallBePresentedDirectlyAfterwards)
	{
		auto& destinationImage = image_for_frame(aDestinationFrameId);
		return blit_image(aSourceImage, image_for_frame(aDestinationFrameId), cgb::sync::with_barriers_by_return(
			cgb::sync::presets::image_copy::wait_for_previous_operations(aSourceImage, destinationImage),
			aShallBePresentedDirectlyAfterwards 
				? cgb::sync::presets::image_copy::directly_into_present(aSourceImage, destinationImage)
				: cgb::sync::presets::image_copy::let_subsequent_operations_wait(aSourceImage, destinationImage)
			),	
			true, // Restore layout of source image
			false // Don't restore layout of destination => this is handled by the after-handler in any case
		);
	}
	
}