namespace cgb
{
	attachment attachment::create_color(image_format pFormat, std::optional<uint32_t> pLocation)
	{
#if defined(_DEBUG)
		if (is_depth_format(pFormat)) {
			LOG_WARNING("The specified image_format is a depth format, but is used for a color attachment.");
		}
#endif
		return attachment{
			pLocation,
			pFormat,
			cfg::attachment_load_operation::clear,
			cfg::attachment_store_operation::store,
			1,				// num samples
			true,		// => present enabled
			false,		// => not depth
			false,			// => no need to resolve
			false	// => not a shader input attachment
		};
	}

	attachment attachment::create_depth(image_format pFormat, std::optional<uint32_t> pLocation)
	{
#if defined(_DEBUG)
		if (!is_depth_format(pFormat)) {
			LOG_WARNING("The specified image_format is probably not a depth format, but is used for a depth attachment.");
		}
#endif
		return attachment{
			pLocation,
			pFormat,
			cfg::attachment_load_operation::clear,
			cfg::attachment_store_operation::store,
			1,				// num samples
			false,		// => present disabled
			true,		// => is depth
			false,			// => no need to resolve
			false	// => not a shader input attachment
		};
	}

	attachment attachment::create_shader_input(image_format pFormat, std::optional<uint32_t> pLocation)
	{
		return attachment{
			pLocation,
			pFormat,
			cfg::attachment_load_operation::load,
			cfg::attachment_store_operation::store,
			1,				// num samples
			false,		// => present disabled
			false,		// => not depth
			false,			// => no need to resolve
			true	// => is a shader input attachment
		};
	}

	attachment attachment::create_color_multisampled(image_format pFormat, int pSampleCount, bool pResolveMultisamples, std::optional<uint32_t> pLocation)
	{
#if defined(_DEBUG)
		if (!is_depth_format(pFormat)) {
			LOG_WARNING("The specified image_format is a depth format, but is used for a color attachment.");
		}
#endif
		return attachment{
			pLocation,
			pFormat,
			cfg::attachment_load_operation::clear,
			cfg::attachment_store_operation::store,
			pSampleCount,				// num samples
			true,		// => present enabled
			false,		// => not depth
			pResolveMultisamples,		// do it or don't?
			false	// => not a shader input attachment
		};
	}

	attachment attachment::create_depth_multisampled(image_format pFormat, int pSampleCount, bool pResolveMultisamples, std::optional<uint32_t> pLocation)
	{
#if defined(_DEBUG)
		if (!is_depth_format(pFormat)) {
			LOG_WARNING("The specified image_format is probably not a depth format, but is used for a depth attachment.");
		}
#endif
		return attachment{
			pLocation,
			pFormat,
			cfg::attachment_load_operation::clear,
			cfg::attachment_store_operation::store,
			pSampleCount,				// num samples
			false,		// => present disabled
			true,		// => is depth
			pResolveMultisamples,		// do it or don't?
			false	// => not a shader input attachment
		};
	}

	attachment attachment::create_shader_input_multisampled(image_format pFormat, int pSampleCount, bool pResolveMultisamples, std::optional<uint32_t> pLocation)
	{
		return attachment{
			pLocation,
			pFormat,
			cfg::attachment_load_operation::load,
			cfg::attachment_store_operation::store,
			pSampleCount,				// num samples
			false,		// => present disabled
			false,		// => not depth
			pResolveMultisamples,		// do it or don't?
			true	// => is a shader input attachment
		};
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