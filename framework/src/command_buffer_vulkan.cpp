namespace cgb
{
	std::vector<command_buffer> command_buffer::create_many(uint32_t pCount, command_pool& pPool, vk::CommandBufferUsageFlags pUsageFlags)
	{
		auto bufferAllocInfo = vk::CommandBufferAllocateInfo()
			.setCommandPool(pPool.handle())
			.setLevel(vk::CommandBufferLevel::ePrimary) // Those, allocated from a pool, are primary command buffers; secondary command buffers can be allocated from command buffers.
			.setCommandBufferCount(pCount);

		auto tmp = context().logical_device().allocateCommandBuffersUnique(bufferAllocInfo);

		// Iterate over all the "raw"-Vk objects in `tmp` and...
		std::vector<command_buffer> buffers;
		buffers.reserve(pCount);
		std::transform(std::begin(tmp), std::end(tmp),
			std::back_inserter(buffers),
			// ...transform them into `cgb::command_buffer` objects:
			[usageFlags = pUsageFlags](auto& vkCb) {
				command_buffer result;
				result.mBeginInfo = vk::CommandBufferBeginInfo()
					.setFlags(usageFlags)
					.setPInheritanceInfo(nullptr);
				result.mCommandBuffer = std::move(vkCb);
				return result;
			});
		return buffers;
	}

	command_buffer command_buffer::create(command_pool& pPool, vk::CommandBufferUsageFlags pUsageFlags)
	{
		auto result = std::move(command_buffer::create_many(1, pPool, pUsageFlags)[0]);
		return result;
	}

	void command_buffer::begin_recording()
	{
		mCommandBuffer->begin(mBeginInfo);
	}

	void command_buffer::end_recording()
	{
		mCommandBuffer->end();
	}

	void command_buffer::begin_render_pass(const vk::RenderPass& pRenderPass, const vk::Framebuffer& pFramebuffer, const vk::Offset2D& pOffset, const vk::Extent2D& pExtent)
	{
		std::array clearValues = {
			vk::ClearValue(vk::ClearColorValue{ make_array<float>( 0.5f, 0.0f, 0.5f, 1.0f ) }),
			//vk::ClearValue(vk::ClearDepthStencilValue{ 1.0f, 0 })
		};
		// TODO: how to determine the number of attachments => and the number of clear-values? omg...

		auto renderPassBeginInfo = vk::RenderPassBeginInfo()
			.setRenderPass(pRenderPass)
			.setFramebuffer(pFramebuffer)
			.setRenderArea(vk::Rect2D()
				.setOffset(pOffset)
				.setExtent(pExtent))
			.setClearValueCount(static_cast<uint32_t>(clearValues.size()))
			.setPClearValues(clearValues.data());

		mCommandBuffer->beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
		// 2nd parameter: how the drawing commands within the render pass will be provided. It can have one of two values [7]:
		//  - VK_SUBPASS_CONTENTS_INLINE: The render pass commands will be embedded in the primary command buffer itself and no secondary command buffers will be executed.
		//  - VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS : The render pass commands will be executed from secondary command buffers.
	}

	void command_buffer::set_image_barrier(const vk::ImageMemoryBarrier& pBarrierInfo)
	{
		mCommandBuffer->pipelineBarrier(
			vk::PipelineStageFlagBits::eAllCommands,
			vk::PipelineStageFlagBits::eAllCommands,
			vk::DependencyFlags(),
			{}, {}, { pBarrierInfo });
	}

	void command_buffer::copy_image(const image_t& pSource, const vk::Image& pDestination)
	{ // TODO: fix this hack after the RTX-VO!
		auto fullImageOffset = vk::Offset3D(0, 0, 0);
		auto fullImageExtent = pSource.config().extent;
		auto halfImageOffset = vk::Offset3D(0, 0, 0); //vk::Offset3D(pSource.mInfo.extent.width / 2, 0, 0);
		auto halfImageExtent = vk::Extent3D(pSource.config().extent.width / 2, pSource.config().extent.height, pSource.config().extent.depth);
		auto offset = halfImageOffset;
		auto extent = halfImageExtent;

		auto copyInfo = vk::ImageCopy()
			.setSrcSubresource(vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0u, 0u, 1u))
			.setSrcOffset(offset)
			.setDstSubresource(vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0u, 0u, 1u))
			.setDstOffset(offset)
			.setExtent(extent);
		mCommandBuffer->copyImage(pSource.image_handle(), vk::ImageLayout::eTransferSrcOptimal, pDestination, vk::ImageLayout::eTransferDstOptimal, { copyInfo });
	}

	void command_buffer::end_render_pass()
	{
		mCommandBuffer->endRenderPass();
	}
}