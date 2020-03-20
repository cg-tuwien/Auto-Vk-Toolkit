#include <cg_base.hpp>

namespace cgb
{
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
			case cgb::presentation_mode::double_buffering:
				selPresModeItr = std::find(std::begin(presModes), std::end(presModes), vk::PresentModeKHR::eFifoRelaxed);
				break;
			case cgb::presentation_mode::vsync:
				selPresModeItr = std::find(std::begin(presModes), std::end(presModes), vk::PresentModeKHR::eFifo);
				break;
			case cgb::presentation_mode::triple_buffering:
				selPresModeItr = std::find(std::begin(presModes), std::end(presModes), vk::PresentModeKHR::eMailbox);
				break;
			default:
				throw std::runtime_error("should not get here");
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
				throw new std::runtime_error("Failed to create window with the title '" + mTitle + "'");
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
			set_presentaton_mode(cgb::presentation_mode::triple_buffering);
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
		if (!aFrameId.has_value()) {
			aFrameId = current_frame();
		}
		mSingleUseCommandBuffers.emplace_back(aFrameId.value(), std::move(aCommandBuffer));
	}
	
	std::vector<semaphore> window::remove_all_extra_semaphore_dependencies_for_frame(int64_t aFrameId)
	{
		// Find all to remove
		auto to_remove = std::remove_if(
			std::begin(mExtraSemaphoreDependencies), std::end(mExtraSemaphoreDependencies),
			[frameId = aFrameId](const auto& tpl) {
				return std::get<int64_t>(tpl) == frameId;
			});
		// return ownership of all the semaphores to remove to the caller
		std::vector<semaphore> moved_semaphores;
		// TODO: Remove the following line, once the noexcept-horror is fixed:
		moved_semaphores.reserve(mExtraSemaphoreDependencies.size());
		for (decltype(to_remove) it = to_remove; it != std::end(mExtraSemaphoreDependencies); ++it) {
			moved_semaphores.push_back(std::move(std::get<semaphore>(*it)));
		}
		// Erase and return
		mExtraSemaphoreDependencies.erase(to_remove, std::end(mExtraSemaphoreDependencies));
		return moved_semaphores;
	}

	std::vector<command_buffer> window::remove_all_single_use_command_buffers_for_frame(int64_t aFrameId)
	{
		// Find all to remove
		auto to_remove = std::remove_if(
			std::begin(mSingleUseCommandBuffers), std::end(mSingleUseCommandBuffers),
			[frameId = aFrameId](const auto& tpl) {
				return std::get<int64_t>(tpl) == frameId;
			});
		// return ownership of all the command_buffers to remove to the caller
		std::vector<command_buffer> moved_command_buffers;
		for (decltype(to_remove) it = to_remove; it != std::end(mSingleUseCommandBuffers); ++it) {
			moved_command_buffers.push_back(std::move(std::get<command_buffer>(*it)));
		}
		// Erase and return
		mSingleUseCommandBuffers.erase(to_remove, std::end(mSingleUseCommandBuffers));
		return moved_command_buffers;
	}

	void window::fill_in_extra_semaphore_dependencies_for_frame(std::vector<vk::Semaphore>& aSemaphores, int64_t aFrameId)
	{
		for (const auto& [frameId, sem] : mExtraSemaphoreDependencies) {
			if (frameId == aFrameId) {
				aSemaphores.push_back(sem->handle());
			}
		}
	}

	void window::fill_in_extra_render_finished_semaphores_for_frame(std::vector<vk::Semaphore>& aSemaphores, int64_t aFrameId)
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

	void window::render_frame(std::vector<std::reference_wrapper<const cgb::command_buffer_t>> aCommandBufferRefs, std::optional<std::reference_wrapper<const cgb::image_t>> aCopyToPresent)
	{
		vk::Result result;

		// Wait for the fence before proceeding, GPU -> CPU synchronization via fence
		const auto& fence = fence_for_frame();
		cgb::context().logical_device().waitForFences(1u, fence.handle_addr(), VK_TRUE, std::numeric_limits<uint64_t>::max());
		result = cgb::context().logical_device().resetFences(1u, fence.handle_addr());
		assert (vk::Result::eSuccess == result);

		// At this point we are certain that frame which used the current fence before is done.
		//  => Clean up the resources of that previous frame!
		auto semaphoresToBeFreed 
			= remove_all_extra_semaphore_dependencies_for_frame(current_frame() - number_of_in_flight_frames());
		auto commandBuffersToBeFreed 
			= remove_all_single_use_command_buffers_for_frame(current_frame() - number_of_in_flight_frames());

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

		std::vector<vk::CommandBuffer> cmdBuffers;
		for (auto cb : aCommandBufferRefs) {
			cmdBuffers.push_back(cb.get().handle());
		}
		if (aCopyToPresent.has_value()) {
			// Create a one-time-submit command buffer. 
			// TODO: Cache these command buffers, based on the input image!
			auto cmdbfr = cgb::context().graphics_queue().create_single_use_command_buffer();
			cmdbfr->begin_recording();
			cmdbfr->handle().pipelineBarrier(
				vk::PipelineStageFlagBits::eAllCommands,
				vk::PipelineStageFlagBits::eAllCommands,
				vk::DependencyFlags(),
				{}, {}, {
					vk::ImageMemoryBarrier(
						vk::AccessFlags(), 
						vk::AccessFlagBits::eTransferWrite,
						vk::ImageLayout::eUndefined,
						vk::ImageLayout::eTransferDstOptimal,
						VK_QUEUE_FAMILY_IGNORED,
						VK_QUEUE_FAMILY_IGNORED,
						mSwapChainImages[imageIndex],
						vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0u, 1u, 0u, 1u)
					)
				});

			cmdbfr->copy_image(aCopyToPresent.value().get(), mSwapChainImages[imageIndex]);

			cmdbfr->handle().pipelineBarrier(
				vk::PipelineStageFlagBits::eAllCommands,
				vk::PipelineStageFlagBits::eAllCommands,
				vk::DependencyFlags(),
				{}, {}, {
					vk::ImageMemoryBarrier(
						vk::AccessFlags(), 
						vk::AccessFlagBits::eTransferWrite,
						vk::ImageLayout::eTransferDstOptimal,
						vk::ImageLayout::ePresentSrcKHR,
						VK_QUEUE_FAMILY_IGNORED,
						VK_QUEUE_FAMILY_IGNORED,
						mSwapChainImages[imageIndex],
						vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0u, 1u, 0u, 1u)
					)
				});
			cmdbfr->end_recording();

			cmdBuffers.push_back(cmdbfr->handle());
			handle_single_use_command_buffer_lifetime(std::move(cmdbfr));
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

}