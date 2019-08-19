#include <cg_base.hpp>

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

	attachment attachment::create_depth(std::optional<image_format> pFormat, std::optional<uint32_t> pLocation)
	{
		if (!pFormat.has_value()) {
			pFormat = image_format::default_depth_format();
		}

#if defined(_DEBUG)
		if (!is_depth_format(pFormat.value())) {
			LOG_WARNING("The specified image_format is probably not a depth format, but is used for a depth attachment.");
		}
#endif
		return attachment{
			pLocation,
			pFormat.value(),
			cfg::attachment_load_operation::clear,
			cfg::attachment_store_operation::store,
			1,				// num samples
			false,		// => present disabled
			true,		// => is depth
			false,			// => no need to resolve
			false	// => not a shader input attachment
		};
	}

	attachment attachment::create_depth_stencil(std::optional<image_format> pFormat, std::optional<uint32_t> pLocation)
	{
		if (!pFormat.has_value()) {
			pFormat = image_format::default_depth_stencil_format();
		}
		return create_depth(std::move(pFormat), std::move(pLocation));
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
}