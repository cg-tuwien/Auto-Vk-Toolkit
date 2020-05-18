#include <cg_base.hpp>

namespace cgb
{
	image_view_as_input_attachment image_view_t::as_input_attachment() const
	{
		image_view_as_input_attachment result;
		result.mDescriptorInfo = vk::DescriptorImageInfo{}
			.setImageView(handle())
			.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal); // TODO: This SHOULD be the most common layout for input attachments... but can it also not be?
		return result;
	}

	image_view_as_storage_image image_view_t::as_storage_image() const
	{
		image_view_as_storage_image result;
		result.mDescriptorInfo = vk::DescriptorImageInfo{}
			.setImageView(handle())
			.setImageLayout(get_image().target_layout()); // TODO: Can it also be desired NOT to use the image's target_layout? 
		return result;
	}
	
	owning_resource<image_view_t> image_view_t::create(cgb::image aImageToOwn, std::optional<image_format> aViewFormat, context_specific_function<void(image_view_t&)> aAlterConfigBeforeCreation)
	{
		image_view_t result;
		
		// Transfer ownership:
		result.mImage = std::move(aImageToOwn);

		// What's the format of the image view?
		if (!aViewFormat) {
			aViewFormat = image_format(result.get_image().format());
		}

		result.finish_configuration(*aViewFormat, {}, std::move(aAlterConfigBeforeCreation));
		
		return result;
	}

	owning_resource<image_view_t> image_view_t::create_depth(cgb::image aImageToOwn, std::optional<image_format> aViewFormat, context_specific_function<void(image_view_t&)> aAlterConfigBeforeCreation)
	{
		image_view_t result;
		
		// Transfer ownership:
		result.mImage = std::move(aImageToOwn);

		// What's the format of the image view?
		if (!aViewFormat) {
			aViewFormat = image_format(result.get_image().format());
		}

		result.finish_configuration(*aViewFormat, vk::ImageAspectFlagBits::eDepth, std::move(aAlterConfigBeforeCreation));
		
		return result;
	}

	owning_resource<image_view_t> image_view_t::create_stencil(cgb::image aImageToOwn, std::optional<image_format> aViewFormat, context_specific_function<void(image_view_t&)> aAlterConfigBeforeCreation)
	{
		image_view_t result;
		
		// Transfer ownership:
		result.mImage = std::move(aImageToOwn);

		// What's the format of the image view?
		if (!aViewFormat) {
			aViewFormat = image_format(result.get_image().format());
		}

		result.finish_configuration(*aViewFormat, vk::ImageAspectFlagBits::eStencil, std::move(aAlterConfigBeforeCreation));
		
		return result;
	}

	owning_resource<image_view_t> image_view_t::create(cgb::image_t aImageToWrap, std::optional<image_format> aViewFormat)
	{
		image_view_t result;
		
		// Transfer ownership:
		result.mImage = image_view_t::helper_t( std::move(aImageToWrap) );

		// What's the format of the image view?
		if (!aViewFormat) {
			aViewFormat = image_format(result.get_image().format());
		}

		result.finish_configuration(*aViewFormat, {}, nullptr);
		
		return result;
	}

	void image_view_t::finish_configuration(image_format aViewFormat, std::optional<vk::ImageAspectFlags> aImageAspectFlags, context_specific_function<void(image_view_t&)> _AlterConfigBeforeCreation)
	{
		if (!aImageAspectFlags.has_value()) {
			const auto imageFormat = image_format(get_image().config().format);
			aImageAspectFlags = get_image().aspect_flags();
			
			if (is_depth_format(imageFormat)) {
				if (has_stencil_component(imageFormat)) {
					LOG_ERROR("Can infer whether the image view shall refer to the depth component or to the stencil component => State it explicitly by using image_view_t::create_depth or image_view_t::create_stencil");
				}
				aImageAspectFlags = vk::ImageAspectFlagBits::eDepth;
				// TODO: use vk::ImageAspectFlagBits' underlying type and exclude eStencil rather than only setting eDepth!
			}
			else if(has_stencil_component(imageFormat)) {
				aImageAspectFlags = vk::ImageAspectFlagBits::eStencil;
				// TODO: use vk::ImageAspectFlagBits' underlying type and exclude eDepth rather than only setting eStencil!
			}
		}
		
		// Proceed with config creation (and use the imageAspectFlags there):
		mInfo = vk::ImageViewCreateInfo{}
			.setImage(get_image().handle())
			.setViewType(to_image_view_type(get_image().config()))
			.setFormat(aViewFormat.mFormat)
			.setComponents(vk::ComponentMapping() // The components field allows you to swizzle the color channels around. In our case we'll stick to the default mapping. [3]
							  .setR(vk::ComponentSwizzle::eIdentity)
							  .setG(vk::ComponentSwizzle::eIdentity)
							  .setB(vk::ComponentSwizzle::eIdentity)
							  .setA(vk::ComponentSwizzle::eIdentity))
			.setSubresourceRange(vk::ImageSubresourceRange() // The subresourceRange field describes what the image's purpose is and which part of the image should be accessed. Our images will be used as color targets without any mipmapping levels or multiple layers. [3]
				.setAspectMask(aImageAspectFlags.value())
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
			.setImageView(handle())
			.setImageLayout(get_image().target_layout()); // TODO: Better use the image's current layout or its target layout? 

		mTracker.setTrackee(*this);
	}

}
