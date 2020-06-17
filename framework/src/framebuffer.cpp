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

		// Set the right layouts for the images:
		const auto n = result.mImageViews.size();
		const auto& attDescs = result.mRenderpass->attachment_descriptions();
		for (size_t i = 0; i < n; ++i) {
			result.mImageViews[i]->get_image().transition_to_layout(attDescs[i].initialLayout);
		}
		
		return result;
	}

	owning_resource<framebuffer_t> framebuffer_t::create(std::vector<xv::attachment> aAttachments, std::vector<image_view> aImageViews, uint32_t aWidth, uint32_t aHeight, cgb::context_specific_function<void(framebuffer_t&)> aAlterConfigBeforeCreation)
	{
		check_and_config_attachments_based_on_views(aAttachments, aImageViews);
		return create(
			renderpass_t::create(std::move(aAttachments)),
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

	owning_resource<framebuffer_t> framebuffer_t::create(std::vector<xv::attachment> aAttachments, std::vector<image_view> aImageViews, cgb::context_specific_function<void(framebuffer_t&)> aAlterConfigBeforeCreation)
	{
		check_and_config_attachments_based_on_views(aAttachments, aImageViews);
		return create(
			std::move(renderpass_t::create(std::move(aAttachments))),
			std::move(aImageViews),
			std::move(aAlterConfigBeforeCreation)
		);
	}

	std::optional<command_buffer> framebuffer_t::initialize_attachments(cgb::sync aSync)
	{
		aSync.set_queue_hint(cgb::context().graphics_queue()); // TODO: Transfer queue?!
		aSync.establish_barrier_before_the_operation(pipeline_stage::transfer, {}); // TODO: Don't use transfer after barrier-stage-refactoring
		
		const int n = mImageViews.size();
		assert (n == mRenderpass->attachment_descriptions().size());
		for (size_t i = 0; i < n; ++i) {
			mImageViews[i]->get_image().transition_to_layout(mRenderpass->attachment_descriptions()[i].finalLayout, cgb::sync::auxiliary_with_barriers(aSync, {}, {}));
		}

		aSync.establish_barrier_after_the_operation(pipeline_stage::transfer, {}); // TODO: Don't use transfer after barrier-stage-refactoring
		return aSync.submit_and_sync();
	}

	void framebuffer_t::check_and_config_attachments_based_on_views(std::vector<xv::attachment>& aAttachments, std::vector<image_view>& aImageViews)
	{
		if (aAttachments.size() != aImageViews.size()) {
			throw cgb::runtime_error(fmt::format("Incomplete config for framebuffer creation: number of attachments ({}) does not equal the number of image views ({})", aAttachments.size(), aImageViews.size()));
		}
		auto n = aAttachments.size();
		for (size_t i = 0; i < n; ++i) {
			auto& a = aAttachments[i];
			auto& v = aImageViews[i];
			if ((cgb::is_depth_format(v->get_image().format()) || cgb::has_stencil_component(v->get_image().format())) && !a.is_used_as_depth_stencil_attachment()) {
				LOG_WARNING(fmt::format("Possibly misconfigured framebuffer: image[{}] is a depth/stencil format, but it is never indicated to be used as such in the attachment-description[{}].", i, i));
			}
			// TODO: Maybe further checks?
			if (!a.mImageUsageHintBefore.has_value() && !a.mImageUsageHintAfter.has_value()) {
				a.mImageUsageHintAfter = a.mImageUsageHintBefore = v->get_image().usage_config();
			}
		}
	}
}
