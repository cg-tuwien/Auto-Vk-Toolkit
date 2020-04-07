#include <cg_base.hpp>

namespace cgb
{
	command_buffer_t::~command_buffer_t()
	{
		if (mCustomDeleter.has_value() && *mCustomDeleter) {
			// If there is a custom deleter => call it now
			(*mCustomDeleter)();
			mCustomDeleter.reset();
		}
		// Destroy the dependant instance before destroying myself
		// ^ This is ensured by the order of the members
		//   See: https://isocpp.org/wiki/faq/dtors#calling-member-dtors
	}
	
	std::vector<owning_resource<command_buffer_t>> command_buffer_t::create_many(uint32_t aCount, command_pool& aPool, vk::CommandBufferUsageFlags aUsageFlags)
	{
		auto bufferAllocInfo = vk::CommandBufferAllocateInfo()
			.setCommandPool(aPool.handle())
			.setLevel(vk::CommandBufferLevel::ePrimary) // Those, allocated from a pool, are primary command buffers; secondary command buffers can be allocated from command buffers.
			.setCommandBufferCount(aCount);

		auto tmp = context().logical_device().allocateCommandBuffersUnique(bufferAllocInfo);

		// Iterate over all the "raw"-Vk objects in `tmp` and...
		std::vector<owning_resource<command_buffer_t>> buffers;
		buffers.reserve(aCount);
		std::transform(std::begin(tmp), std::end(tmp),
			std::back_inserter(buffers),
			// ...transform them into `cgb::command_buffer_t` objects:
			[lUsageFlags = aUsageFlags](auto& vkCb) -> owning_resource<command_buffer_t> {
				command_buffer_t result;
				result.mBeginInfo = vk::CommandBufferBeginInfo()
					.setFlags(lUsageFlags)
					.setPInheritanceInfo(nullptr);
				result.mCommandBuffer = std::move(vkCb);
				return result;
			});
		return buffers;
	}

	owning_resource<command_buffer_t> command_buffer_t::create(command_pool& aPool, vk::CommandBufferUsageFlags aUsageFlags)
	{
		auto result = std::move(command_buffer_t::create_many(1, aPool, aUsageFlags)[0]);
		return result;
	}

	void command_buffer_t::begin_recording()
	{
		mCommandBuffer->begin(mBeginInfo);
		mState = command_buffer_state::recording;
	}

	void command_buffer_t::end_recording()
	{
		mCommandBuffer->end();
		mState = command_buffer_state::finished_recording;
	}

	void command_buffer_t::begin_render_pass(const graphics_pipeline_t& aPipeline, window* aWindow, std::optional<int64_t> aSwapchainImageIndex)
	{
		const auto& rp = aPipeline.get_renderpass();
		const auto subpassId = aPipeline.subpass_id();
		begin_render_pass(rp, subpassId, aWindow, aSwapchainImageIndex);
	}

	void command_buffer_t::begin_render_pass(const renderpass_t& aRenderpass, uint32_t aSubpassId, window* aWindow, std::optional<int64_t> aSwapchainImageIndex)
	{
		auto extent = cgb::context().main_window()->swap_chain_extent();
		const auto inFlightIndex = aSwapchainImageIndex.value_or(aWindow->swapchain_image_index_for_frame());
		auto backbufferHandle = cgb::context().main_window()->backbuffer_at_index(inFlightIndex)->handle();

		std::vector<vk::ClearValue> clearValues;
		auto& rpAttachments = aRenderpass.attachment_descriptions();
		clearValues.reserve(rpAttachments.size());
		for (size_t i = 0; i < rpAttachments.size(); ++i) {
			if (aRenderpass.is_color_attachment(aSubpassId, i)) {
				clearValues.push_back(vk::ClearValue(vk::ClearColorValue{ make_array<float>(0.5f, 0.0f, 0.5f, 1.0f) }));
				continue;
			}
			if (aRenderpass.is_depth_stencil_attachment(aSubpassId, i)) {
				clearValues.push_back(vk::ClearValue(vk::ClearDepthStencilValue{ 1.0f, 0 }));
				continue;
			}
			if (aRenderpass.is_input_attachment(aSubpassId, i)) {
				clearValues.push_back(vk::ClearValue(/* TODO: What to do? */));
				continue;
			}
			if (aRenderpass.is_resolve_attachment(aSubpassId, i)) {
				clearValues.push_back(vk::ClearValue(/* TODO: What to do? */));
				continue;
			}
		}

		// Begin render aass and clear the attachments (color and depth)
		begin_render_pass_for_framebuffer(aRenderpass.handle(), backbufferHandle, { 0, 0 }, extent, std::move(clearValues));
	}
	
	void command_buffer_t::begin_render_pass_for_framebuffer(const vk::RenderPass& aRenderPass, const vk::Framebuffer& aFramebuffer, const vk::Offset2D& aOffset, const vk::Extent2D& aExtent, std::vector<vk::ClearValue> aClearValues)
	{
		auto renderPassBeginInfo = vk::RenderPassBeginInfo()
			.setRenderPass(aRenderPass)
			.setFramebuffer(aFramebuffer)
			.setRenderArea(vk::Rect2D()
				.setOffset(aOffset)
				.setExtent(aExtent))
			.setClearValueCount(static_cast<uint32_t>(aClearValues.size()))
			.setPClearValues(aClearValues.data());

		mCommandBuffer->beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
		// 2nd parameter: how the drawing commands within the render pass will be provided. It can have one of two values [7]:
		//  - VK_SUBPASS_CONTENTS_INLINE: The render pass commands will be embedded in the primary command buffer itself and no secondary command buffers will be executed.
		//  - VK_SUBPASS_CONTENTS_SECONDARY_command_buffer_tS : The render pass commands will be executed from secondary command buffers.
	}

	void command_buffer_t::begin_render_pass(const renderpass_t& aRenderpass, uint32_t aSubpassId, const framebuffer_t& aFramebuffer)
	{
		assert(!aFramebuffer.image_views().empty());

		std::vector<vk::ClearValue> clearValues;
		auto& rpAttachments = aRenderpass.attachment_descriptions();
		clearValues.reserve(rpAttachments.size());
		for (size_t i = 0; i < rpAttachments.size(); ++i) {
			if (aRenderpass.is_color_attachment(aSubpassId, i)) {
				clearValues.push_back(vk::ClearValue(vk::ClearColorValue{ make_array<float>(0.5f, 0.0f, 0.5f, 1.0f) }));
				continue;
			}
			if (aRenderpass.is_depth_stencil_attachment(aSubpassId, i)) {
				clearValues.push_back(vk::ClearValue(vk::ClearDepthStencilValue{ 1.0f, 0 }));
				continue;
			}
			if (aRenderpass.is_input_attachment(aSubpassId, i)) {
				clearValues.push_back(vk::ClearValue(/* TODO: What to do? */));
				continue;
			}
			if (aRenderpass.is_resolve_attachment(aSubpassId, i)) {
				clearValues.push_back(vk::ClearValue(/* TODO: What to do? */));
				continue;
			}
		}
		
		auto extent = aFramebuffer.image_view_at(0)->get_image().config().extent;
		begin_render_pass_for_framebuffer(aRenderpass.handle(), aFramebuffer.handle(), { 0, 0 }, vk::Extent2D{ extent.width, extent.height }, std::move(clearValues));
	}
	
	void command_buffer_t::begin_render_pass(const framebuffer_t& aFramebuffer)
	{
		begin_render_pass(aFramebuffer.get_renderpass(), 0u, aFramebuffer);
	}

	void command_buffer_t::establish_execution_barrier(pipeline_stage aSrcStage, pipeline_stage aDstStage)
	{
		mCommandBuffer->pipelineBarrier(
			to_vk_pipeline_stage_flags(aSrcStage), // Up to which stage to execute before making memory available
			to_vk_pipeline_stage_flags(aDstStage), // Which stage has to wait until memory has been made visible
			vk::DependencyFlags{}, // TODO: support dependency flags
			{},	{}, {} // no memory barriers
		);
	}

	void command_buffer_t::establish_global_memory_barrier(pipeline_stage aSrcStage, pipeline_stage aDstStage, std::optional<memory_access> aSrcAccessToBeMadeAvailable, std::optional<memory_access> aDstAccessToBeMadeVisible)
	{
		mCommandBuffer->pipelineBarrier(
			to_vk_pipeline_stage_flags(aSrcStage),				// Up to which stage to execute before making memory available
			to_vk_pipeline_stage_flags(aDstStage),				// Which stage has to wait until memory has been made visible
			vk::DependencyFlags{},								// TODO: support dependency flags
			{ vk::MemoryBarrier{								// Establish a global memory barrier, ...
				to_vk_access_flags(aSrcAccessToBeMadeAvailable),//  ... making memory from these access types available (after aSrcStage),
				to_vk_access_flags(aDstAccessToBeMadeVisible)	//  ... and for these access types visible (before aDstStage)
			}},
			{}, {} // no buffer/image memory barriers
		);
	}

	void command_buffer_t::establish_global_memory_barrier(pipeline_stage aSrcStage, pipeline_stage aDstStage, std::optional<write_memory_access> aSrcAccessToBeMadeAvailable, std::optional<read_memory_access> aDstAccessToBeMadeVisible)
	{
		establish_global_memory_barrier(aSrcStage, aDstStage, to_memory_access(aSrcAccessToBeMadeAvailable), to_memory_access(aDstAccessToBeMadeVisible));
	}

	void command_buffer_t::establish_image_memory_barrier(image_t& aImage, pipeline_stage aSrcStage, pipeline_stage aDstStage, std::optional<memory_access> aSrcAccessToBeMadeAvailable, std::optional<memory_access> aDstAccessToBeMadeVisible)
	{
		mCommandBuffer->pipelineBarrier(
			to_vk_pipeline_stage_flags(aSrcStage),						// Up to which stage to execute before making memory available
			to_vk_pipeline_stage_flags(aDstStage),						// Which stage has to wait until memory has been made visible
			vk::DependencyFlags{},										// TODO: support dependency flags
			{}, {},														// no global memory barriers, no buffer memory barriers
			{
				vk::ImageMemoryBarrier{
					to_vk_access_flags(aSrcAccessToBeMadeAvailable),	// After the aSrcStage, make this memory available
					to_vk_access_flags(aDstAccessToBeMadeVisible),		// Before the aDstStage, make this memory visible
					aImage.current_layout(), aImage.target_layout(),	// Transition for the former to the latter
					VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,	// TODO: Support queue family ownership transfer
					aImage.handle(),
					aImage.entire_subresource_range()					// TODO: Support different subresource ranges
				}
			}
		);
		aImage.set_current_layout(aImage.target_layout()); // Just optimistically set it
	}
	
	void command_buffer_t::establish_image_memory_barrier(image_t& aImage, pipeline_stage aSrcStage, pipeline_stage aDstStage, std::optional<write_memory_access> aSrcAccessToBeMadeAvailable, std::optional<read_memory_access> aDstAccessToBeMadeVisible)
	{
		establish_image_memory_barrier(aImage, aSrcStage, aDstStage, to_memory_access(aSrcAccessToBeMadeAvailable), to_memory_access(aDstAccessToBeMadeVisible));
	}

	void command_buffer_t::copy_image(const image_t& aSource, const vk::Image& aDestination)
	{ // TODO: fix this hack after the RTX-VO!
		auto fullImageOffset = vk::Offset3D(0, 0, 0);
		auto fullImageExtent = aSource.config().extent;
		auto halfImageOffset = vk::Offset3D(0, 0, 0); //vk::Offset3D(pSource.mInfo.extent.width / 2, 0, 0);
		auto halfImageExtent = vk::Extent3D(aSource.config().extent.width, aSource.config().extent.height, aSource.config().extent.depth);
		auto offset = halfImageOffset;
		auto extent = halfImageExtent;

		auto copyInfo = vk::ImageCopy()
			.setSrcSubresource(vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0u, 0u, 1u))
			.setSrcOffset(offset)
			.setDstSubresource(vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0u, 0u, 1u))
			.setDstOffset(offset)
			.setExtent(extent);
		mCommandBuffer->copyImage(aSource.handle(), vk::ImageLayout::eTransferSrcOptimal, aDestination, vk::ImageLayout::eTransferDstOptimal, { copyInfo });
	}

	void command_buffer_t::end_render_pass()
	{
		mCommandBuffer->endRenderPass();
	}

	void command_buffer_t::bind_descriptors(vk::PipelineBindPoint aBindingPoint, vk::PipelineLayout aLayoutHandle, std::initializer_list<binding_data> aBindings, descriptor_cache_interface* aDescriptorCache)
	{
		if (nullptr == aDescriptorCache) {
			aDescriptorCache = cgb::context().get_standard_descriptor_cache();
		}

		auto dsets = cgb::descriptor_set::get_or_create(std::move(aBindings), aDescriptorCache);

		if (dsets.size() == 0) {
			LOG_WARNING("command_buffer_t::bind_descriptors has been called, but there are no descriptor sets to be bound.");
			return;
		}

		std::vector<vk::DescriptorSet> handles;
		handles.reserve(dsets.size());
		for (const auto& dset : dsets)
		{
			handles.push_back(dset->handle());
		}
		
		handle().bindDescriptorSets(
			aBindingPoint, 
			aLayoutHandle, 
			dsets.front()->set_id(),
			static_cast<uint32_t>(handles.size()),
			handles.data(), 
			0, // TODO: Dynamic offset count
			nullptr); // TODO: Dynamic offset
	}
}