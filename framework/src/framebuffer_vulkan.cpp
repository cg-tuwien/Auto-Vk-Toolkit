namespace cgb
{

	owning_resource<framebuffer_t> framebuffer_t::create(renderpass _Renderpass, std::vector<image_view> _ImageViews, uint32_t _Width, uint32_t _Height, cgb::context_specific_function<void(framebuffer_t&)> _AlterConfigBeforeCreation)
	{
		framebuffer_t result;
		result.mRenderpass = std::move(_Renderpass);
		result.mImageViews = std::move(_ImageViews);

		std::vector<vk::ImageView> imageViewHandles;
		for (const auto& iv : result.mImageViews) {
			imageViewHandles.push_back(iv->view_handle());
		}

		result.mCreateInfo = vk::FramebufferCreateInfo{}
			.setRenderPass(result.mRenderpass->handle())
			.setAttachmentCount(static_cast<uint32_t>(imageViewHandles.size()))
			.setPAttachments(imageViewHandles.data())
			.setWidth(_Width)
			.setHeight(_Height)
			// TODO: Support multiple layers of image arrays!
			.setLayers(1u); // number of layers in image arrays [6]

		// Maybe alter the config?!
		if (_AlterConfigBeforeCreation.mFunction) {
			_AlterConfigBeforeCreation.mFunction(result);
		}

		result.mTracker.setTrackee(result);
		result.mFramebuffer = context().logical_device().createFramebufferUnique(result.mCreateInfo);
		return result;
	}

	owning_resource<framebuffer_t> framebuffer_t::create(std::vector<attachment> pAttachments, std::vector<image_view> _ImageViews, uint32_t _Width, uint32_t _Height, cgb::context_specific_function<void(framebuffer_t&)> _AlterConfigBeforeCreation)
	{
		return framebuffer_t::create(
			renderpass_t::create(std::move(pAttachments)),
			std::move(_ImageViews),
			_Width, _Height,
			std::move(_AlterConfigBeforeCreation)
		);
	}

	owning_resource<framebuffer_t> framebuffer_t::create(std::vector<image_view> _ImageViews, uint32_t _Width, uint32_t _Height, cgb::context_specific_function<void(framebuffer_t&)> _AlterConfigBeforeCreation)
	{
		// Create attachments based on the given image views
		std::vector<attachment> attachments;
		uint32_t  location = 0u;
		for (const auto& iv : _ImageViews) {
			attachments.push_back(attachment::create_for(iv, location++));
		}

		// Hope that helps:
		return framebuffer_t::create(std::move(attachments), std::move(_ImageViews), _Width, _Height, std::move(_AlterConfigBeforeCreation));
	}

}
