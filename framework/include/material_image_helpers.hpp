#pragma once

namespace cgb
{	
	extern vk::ImageMemoryBarrier create_image_barrier(vk::Image pImage, vk::Format pFormat, vk::AccessFlags pSrcAccessMask, vk::AccessFlags pDstAccessMask, vk::ImageLayout pOldLayout, vk::ImageLayout pNewLayout, std::optional<vk::ImageSubresourceRange> pSubresourceRange = std::nullopt);

	extern void transition_image_layout(image_t& _Image, vk::Format _Format, vk::ImageLayout _NewLayout, const semaphore_t* _WaitSemaphore = nullptr, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler = {});

	extern void copy_image_to_another(const image_t& pSrcImage, image_t& pDstImage, const semaphore_t* _WaitSemaphore = nullptr, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler = {});

	template <typename Bfr>
	owning_resource<semaphore_t> copy_buffer_to_image(const Bfr& pSrcBuffer, const image_t& pDstImage, const semaphore_t* _WaitSemaphore = nullptr)
	{
		//auto commandBuffer = context().create_command_buffers_for_transfer(1);
		auto commandBuffer = cgb::context().transfer_queue().pool().get_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

		// Immediately start recording the command buffer:
		commandBuffer.begin_recording();

		auto copyRegion = vk::BufferImageCopy()
			.setBufferOffset(0)
			// The bufferRowLength and bufferImageHeight fields specify how the pixels are laid out in memory. For example, you could have some padding 
			// bytes between rows of the image. Specifying 0 for both indicates that the pixels are simply tightly packed like they are in our case. [3]
			.setBufferRowLength(0)
			.setBufferImageHeight(0)
			.setImageSubresource(vk::ImageSubresourceLayers()
				.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setMipLevel(0u)
				.setBaseArrayLayer(0u)
				.setLayerCount(1u))
			.setImageOffset({ 0u, 0u, 0u })
			.setImageExtent(pDstImage.config().extent);

		commandBuffer.handle().copyBufferToImage(
			pSrcBuffer->buffer_handle(),
			pDstImage.image_handle(),
			vk::ImageLayout::eTransferDstOptimal,
			{ copyRegion });

		// That's all
		commandBuffer.end_recording();

		// Create a semaphore which can, or rather, MUST be used to wait for the results
		auto copyCompleteSemaphore = semaphore_t::create();
		copyCompleteSemaphore->set_designated_queue(cgb::context().graphics_queue()); //< Just store the info

		vk::PipelineStageFlags waitMask = vk::PipelineStageFlagBits::eAllCommands; // Just set to all commands. Don't know if this could be optimized somehow?!
		auto submitInfo = vk::SubmitInfo()
			.setCommandBufferCount(1u)
			.setPCommandBuffers(commandBuffer.handle_addr())
			.setWaitSemaphoreCount(nullptr != _WaitSemaphore ? 1u : 0u)
			.setPWaitSemaphores(nullptr != _WaitSemaphore ? _WaitSemaphore->handle_addr() : nullptr)
			.setPWaitDstStageMask(&waitMask)
			.setSignalSemaphoreCount(1u)
			.setPSignalSemaphores(copyCompleteSemaphore->handle_addr());

		cgb::context().transfer_queue().handle().submit({ submitInfo }, nullptr);

		// Take care of the lifetime of:
		//  - commandBuffer
		copyCompleteSemaphore->set_custom_deleter([
			ownedCommandBuffer{ std::move(commandBuffer) }
		]() { /* Nothing to do here, the destructors will do the cleanup, the lambda is just holding them. */ });
		return copyCompleteSemaphore;
	}

	static image create_image_from_file(const std::string& _Path, cgb::memory_usage _MemoryUsage = cgb::memory_usage::device, cgb::image_usage _ImageUsage = cgb::image_usage::versatile_image, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler = {})
	{
		int width, height, channels;
		stbi_set_flip_vertically_on_load(true);
		stbi_uc* pixels = stbi_load(_Path.c_str(), &width, &height, &channels, STBI_rgb_alpha); // TODO: Support different formats!
		size_t imageSize = width * height * 4;

		if (!pixels) {
			throw std::runtime_error("Couldn't load image using stbi_load");
		}

		auto stagingBuffer = cgb::create_and_fill(
			cgb::generic_buffer_meta::create_from_size(imageSize),
			cgb::memory_usage::host_coherent,
			pixels,
			nullptr,
			vk::BufferUsageFlagBits::eTransferSrc);
			
		stbi_image_free(pixels);

		auto img = cgb::image_t::create(width, height, cgb::image_format(vk::Format::eR8G8B8A8Unorm), false, 1, _MemoryUsage, _ImageUsage);
		// 1. Transition image layout to eTransferDstOptimal
		cgb::transition_image_layout(img, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eTransferDstOptimal, nullptr, [&](semaphore sem1) {
			// 2. Copy buffer to image
			auto sem2 = cgb::copy_buffer_to_image(stagingBuffer, img, &*sem1);
			// 3. Transition image layout to eShaderReadOnlyOptimal and handle the semaphore(s) and resources
			//cgb::transition_image_layout(img, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, &*sem2, [&](semaphore sem3) {
			cgb::transition_image_layout(img, vk::Format::eR8G8B8A8Unorm, img->target_layout(), &*sem2, [&](semaphore sem3) {
				if (_SemaphoreHandler) { // Did the user provide a handler?
					sem3->set_custom_deleter([
						ownBuffer = std::move(stagingBuffer),
						ownSem1 = std::move(sem1),
						ownSem2 = std::move(sem2)
					](){});
					_SemaphoreHandler(std::move(sem3)); // Transfer ownership and be done with it
				}
				else {
					LOG_WARNING("No semaphore handler was provided but a semaphore emerged. Will block the device via waitIdle until the operation has completed.");
					cgb::context().logical_device().waitIdle();
				}
			});
		});
		
		return img;
	}


	extern std::tuple<std::vector<material_gpu_data>, std::vector<image_sampler>> convert_for_gpu_usage(std::vector<cgb::material_config> _MaterialConfigs, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler = {}, cgb::image_usage _ImageUsage = cgb::image_usage::read_only_sampled_image);
}
