namespace cgb
{
	const vk::Image& image_view_t::image_handle() const
	{
		if (std::holds_alternative<cgb::image>(mImage)) {
			return std::get<cgb::image>(mImage)->image_handle();
		}
		//assert(std::holds_alternative<std::tuple<vk::Image, vk::ImageCreateInfo>>(mImage));
		return std::get<vk::Image>(std::get<std::tuple<vk::Image, vk::ImageCreateInfo>>(mImage));	
	}

	const vk::ImageCreateInfo& image_view_t::image_config() const
	{
		if (std::holds_alternative<cgb::image>(mImage)) {
			return std::get<cgb::image>(mImage)->config();
		}
		//assert(std::holds_alternative<std::tuple<vk::Image, vk::ImageCreateInfo>>(mImage));
		return std::get<vk::ImageCreateInfo>(std::get<std::tuple<vk::Image, vk::ImageCreateInfo>>(mImage));
	}

	owning_resource<image_view_t> image_view_t::create(cgb::image _ImageToOwn, std::optional<image_format> _ViewFormat, context_specific_function<void(image_view_t&)> _AlterConfigBeforeCreation)
	{
		image_view_t result;
		
		// Transfer ownership:
		result.mImage = std::move(_ImageToOwn);

		// What's the format of the image view?
		if (!_ViewFormat) {
			_ViewFormat = image_format(result.image_config().format);
		}

		result.finish_configuration(*_ViewFormat, std::move(_AlterConfigBeforeCreation));
		
		return result;
	}

	owning_resource<image_view_t> image_view_t::create(vk::Image _ImageToReference, vk::ImageCreateInfo _ImageInfo, std::optional<image_format> _ViewFormat, context_specific_function<void(image_view_t&)> _AlterConfigBeforeCreation)
	{
		image_view_t result;
		
		// Transfer ownership:
		result.mImage = std::make_tuple(_ImageToReference, _ImageInfo);

		// What's the format of the image view?
		if (!_ViewFormat) {
			_ViewFormat = image_format(result.image_config().format);
		}

		result.finish_configuration(*_ViewFormat, std::move(_AlterConfigBeforeCreation));
		
		return result;
	}

	void image_view_t::finish_configuration(image_format _ViewFormat, context_specific_function<void(image_view_t&)> _AlterConfigBeforeCreation)
	{
		// Config for the image view:
		vk::ImageAspectFlags imageAspectFlags;
		{
			// Guess the vk::ImageAspectFlags:
			auto imageFormat = image_format(image_config().format);
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
		mInfo = vk::ImageViewCreateInfo()
			.setImage(image_handle())
			.setViewType(to_image_view_type(image_config()))
			.setFormat(_ViewFormat.mFormat)
			.setComponents(vk::ComponentMapping() // The components field allows you to swizzle the color channels around. In our case we'll stick to the default mapping. [3]
							  .setR(vk::ComponentSwizzle::eIdentity)
							  .setG(vk::ComponentSwizzle::eIdentity)
							  .setB(vk::ComponentSwizzle::eIdentity)
							  .setA(vk::ComponentSwizzle::eIdentity))
			.setSubresourceRange(vk::ImageSubresourceRange() // The subresourceRange field describes what the image's purpose is and which part of the image should be accessed. Our images will be used as color targets without any mipmapping levels or multiple layers. [3]
				.setAspectMask(imageAspectFlags)
				.setBaseMipLevel(0u)
				.setLevelCount(image_config().mipLevels)
				.setBaseArrayLayer(0u)
				.setLayerCount(image_config().arrayLayers));

		// Maybe alter the config?!
		if (_AlterConfigBeforeCreation.mFunction) {
			_AlterConfigBeforeCreation.mFunction(*this);
		}

		mImageView = context().logical_device().createImageViewUnique(mInfo);
		mDescriptorInfo = vk::DescriptorImageInfo{}
			.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal) // TODO: Set this parameter to the right value!!
			.setImageView(view_handle());
		mDescriptorType = vk::DescriptorType::eStorageImage; // TODO: Is it storage image or sampled image?
		mTracker.setTrackee(*this);
	}

	attachment attachment::create_for(const image_view_t& _ImageView, std::optional<uint32_t> pLocation)
	{
		auto& imageInfo = _ImageView.image_config();
		auto format = image_format{ imageInfo.format };
		if (is_depth_format(format)) {
			if (imageInfo.samples == vk::SampleCountFlagBits::e1) {
				return attachment::create_depth(format, pLocation);
			}
			else {	
				// TODO: Should "is presentable" really be true by default?
				return attachment::create_depth_multisampled(format, to_cgb_sample_count(imageInfo.samples), true, pLocation);
			}
		}
		else { // must be color format
			if (imageInfo.samples == vk::SampleCountFlagBits::e1) {
				return attachment::create_color(format, pLocation);
			}
			else {	
				// TODO: Should "is presentable" really be true by default?
				return attachment::create_color_multisampled(format, to_cgb_sample_count(imageInfo.samples), true, pLocation);
			}
		}
		throw std::runtime_error("Unable to create an attachment for the given image view");
	}
}
