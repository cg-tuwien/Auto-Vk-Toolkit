#include <cg_base.hpp>

namespace cgb
{
	owning_resource<image_view_t> image_view_t::create(cgb::image aImageToOwn, std::optional<image_format> aViewFormat, context_specific_function<void(image_view_t&)> aAlterConfigBeforeCreation)
	{
		image_view_t result;
		
		// Transfer ownership:
		result.mImage = std::move(aImageToOwn);

		// What's the format of the image view?
		if (!aViewFormat) {
			aViewFormat = image_format(result.get_image().format());
		}

		result.finish_configuration(*aViewFormat, std::move(aAlterConfigBeforeCreation));
		
		return result;
	}

	owning_resource<image_view_t> image_view_t::create(cgb::image_t aImageToWrap, std::optional<image_format> aViewFormat)
	{
		image_view_t result;
		
		// Transfer ownership:
		result.mImage = image_view_t::helper_t{ std::move(aImageToWrap) };

		// What's the format of the image view?
		if (!aViewFormat) {
			aViewFormat = image_format(result.get_image().format());
		}

		result.finish_configuration(*aViewFormat, nullptr);
		
		return result;
	}

	void image_view_t::finish_configuration(image_format _ViewFormat, context_specific_function<void(image_view_t&)> _AlterConfigBeforeCreation)
	{
		// Config for the image view:
		vk::ImageAspectFlags imageAspectFlags;
		{
			// Guess the vk::ImageAspectFlags:
			auto imageFormat = image_format(get_image().config().format);
			if (is_depth_format(imageFormat)) {
				imageAspectFlags |= vk::ImageAspectFlagBits::eDepth;
				if (has_stencil_component(imageFormat)) {
					imageAspectFlags |= vk::ImageAspectFlagBits::eStencil;
				}
			}
			else {
				imageAspectFlags |= vk::ImageAspectFlagBits::eColor;
			}
			// vk::ImageAspectFlags handling is probably incomplete => Use _AlterConfigBeforeCreation to adapt the config to your requirements!
		}

		// Proceed with config creation (and use the imageAspectFlags there):
		mInfo = vk::ImageViewCreateInfo{}
			.setImage(get_image().handle())
			.setViewType(to_image_view_type(get_image().config()))
			.setFormat(_ViewFormat.mFormat)
			.setComponents(vk::ComponentMapping() // The components field allows you to swizzle the color channels around. In our case we'll stick to the default mapping. [3]
							  .setR(vk::ComponentSwizzle::eIdentity)
							  .setG(vk::ComponentSwizzle::eIdentity)
							  .setB(vk::ComponentSwizzle::eIdentity)
							  .setA(vk::ComponentSwizzle::eIdentity))
			.setSubresourceRange(vk::ImageSubresourceRange() // The subresourceRange field describes what the image's purpose is and which part of the image should be accessed. Our images will be used as color targets without any mipmapping levels or multiple layers. [3]
				.setAspectMask(imageAspectFlags)
				.setBaseMipLevel(0u)
				.setLevelCount(get_image().config().mipLevels)
				.setBaseArrayLayer(0u)
				.setLayerCount(get_image().config().arrayLayers));

		// Maybe alter the config?!
		if (_AlterConfigBeforeCreation.mFunction) {
			_AlterConfigBeforeCreation.mFunction(*this);
		}

		mImageView = context().logical_device().createImageViewUnique(mInfo);
		mDescriptorInfo = vk::DescriptorImageInfo{}
			.setImageView(handle());

		mDescriptorInfo.setImageLayout(get_image().target_layout()); // Note: The image's current layout might be different to its target layout.

		mDescriptorType = vk::DescriptorType::eStorageImage; // TODO: Is it storage image or sampled image?
		mTracker.setTrackee(*this);
	}

	attachment attachment::create_for(const image_view_t& _ImageView, std::optional<uint32_t> pLocation)
	{
		auto& imageInfo = _ImageView.get_image().config();
		auto format = image_format{ imageInfo.format };
		std::optional<image_usage> imageUsage = _ImageView.get_image().usage_config();
		if (is_depth_format(format)) {
			if (imageInfo.samples == vk::SampleCountFlagBits::e1) {
				
				return attachment::create_depth(format, imageUsage.value_or(image_usage::depth_stencil_attachment), pLocation);
			}
			else {	
				// TODO: Should "is presentable" really be true by default? => set to false, maybe it should be handled via a barrier
				return attachment::create_depth_multisampled(format, to_cgb_sample_count(imageInfo.samples), false, imageUsage.value_or(image_usage::depth_stencil_attachment), pLocation);
			}
		}
		else { // must be color format
			if (imageInfo.samples == vk::SampleCountFlagBits::e1) {
				return attachment::create_color(format, imageUsage.value_or(image_usage::color_attachment), pLocation);
			}
			else {	
				// TODO: Should "is presentable" really be true by default? => set to false, maybe it should be handled via a barrier
				return attachment::create_color_multisampled(format, to_cgb_sample_count(imageInfo.samples), false, imageUsage.value_or(image_usage::color_attachment), pLocation);
			}
		}
		throw std::runtime_error("Unable to create an attachment for the given image view");
	}
}
