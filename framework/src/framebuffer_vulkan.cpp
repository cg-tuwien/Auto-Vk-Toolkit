#include <cg_base.hpp>

namespace cgb
{

	owning_resource<framebuffer_t> framebuffer_t::create(renderpass aRenderpass, std::vector<image_view> aImageViews, uint32_t aWidth, uint32_t aHeight, cgb::context_specific_function<void(framebuffer_t&)> _AlterConfigBeforeCreation)
	{
		framebuffer_t result;
		result.mRenderpass = std::move(aRenderpass);
		result.mImageViews = std::move(aImageViews);

		std::vector<vk::ImageView> imageViewHandles;
		for (const auto& iv : result.mImageViews) {
			imageViewHandles.push_back(iv->handle());
		}

		result.mCreateInfo = vk::FramebufferCreateInfo{}
			.setRenderPass(result.mRenderpass->handle())
			.setAttachmentCount(static_cast<uint32_t>(imageViewHandles.size()))
			.setPAttachments(imageViewHandles.data())
			.setWidth(aWidth)
			.setHeight(aHeight)
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

	owning_resource<framebuffer_t> framebuffer_t::create(std::vector<attachment> pAttachments, std::vector<image_view> aImageViews, uint32_t aWidth, uint32_t aHeight, cgb::context_specific_function<void(framebuffer_t&)> aAlterConfigBeforeCreation)
	{
		return create(
			renderpass_t::create(std::move(pAttachments)),
			std::move(aImageViews),
			aWidth, aHeight,
			std::move(aAlterConfigBeforeCreation)
		);
	}

	owning_resource<framebuffer_t> framebuffer_t::create(renderpass aRenderpass, std::vector<image_view> aImageViews, cgb::context_specific_function<void(framebuffer_t&)> aAlterConfigBeforeCreation)
	{
		assert(!aImageViews.empty());
		auto extent = aImageViews.front()->get_image().config().extent;
		return create(std::move(aRenderpass), std::move(aImageViews), extent.width, extent.height, std::move(aAlterConfigBeforeCreation));
	}

	owning_resource<framebuffer_t> framebuffer_t::create(std::vector<attachment> aAttachments, std::vector<image_view> aImageViews, cgb::context_specific_function<void(framebuffer_t&)> aAlterConfigBeforeCreation)
	{
		return create(
			std::move(renderpass_t::create(std::move(aAttachments))),
			std::move(aImageViews),
			std::move(aAlterConfigBeforeCreation)
		);
	}
	
}
