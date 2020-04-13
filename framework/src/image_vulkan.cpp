#include <cg_base.hpp>

#include <set>

namespace cgb
{
	image_format::image_format() noexcept
	{ }

	image_format::image_format(const vk::Format& pFormat) noexcept
		: mFormat{ pFormat }
	{ }

	image_format::image_format(const vk::SurfaceFormatKHR& pSrfFrmt) noexcept 
		: mFormat{ pSrfFrmt.format }
	{ }

	image_format image_format::default_rgb8_4comp_format() noexcept
	{
		return { vk::Format::eR8G8B8A8Unorm };
	}
	
	image_format image_format::default_rgb8_3comp_format() noexcept
	{
		return { vk::Format::eR8G8B8Unorm };
	}
	
	image_format image_format::default_rgb8_2comp_format() noexcept
	{
		return { vk::Format::eR8G8Unorm };
	}
	
	image_format image_format::default_rgb8_1comp_format() noexcept
	{
		return { vk::Format::eR8Unorm };
	}
	
	image_format image_format::default_srgb_4comp_format() noexcept
	{
		return { vk::Format::eR8G8B8A8Srgb };
	}
	
	image_format image_format::default_srgb_3comp_format() noexcept
	{
		return { vk::Format::eR8G8B8Srgb };
	}
	
	image_format image_format::default_srgb_2comp_format() noexcept
	{
		return { vk::Format::eR8G8Srgb };
	}
	
	image_format image_format::default_srgb_1comp_format() noexcept
	{
		return { vk::Format::eR8Srgb };
	}
	
	image_format image_format::default_rgb16f_4comp_format() noexcept
	{
		return { vk::Format::eR16G16B16A16Sfloat };
	}
	
	image_format image_format::default_rgb16f_3comp_format() noexcept
	{
		return { vk::Format::eR16G16B16Sfloat };
	}
	
	image_format image_format::default_rgb16f_2comp_format() noexcept
	{
		return { vk::Format::eR16G16Sfloat };
	}
	
	image_format image_format::default_rgb16f_1comp_format() noexcept
	{
		return { vk::Format::eR16Sfloat };
	}
	

	image_format image_format::default_depth_format() noexcept
	{
		const auto formatCandidates = cgb::make_array<vk::Format>(
			vk::Format::eD32Sfloat,
			vk::Format::eD24UnormS8Uint,
			vk::Format::eD16Unorm,
			vk::Format::eD16UnormS8Uint,
			vk::Format::eD32SfloatS8Uint
		);

		if (cgb::context().state() < context_state::fully_initialized) {
			return formatCandidates[1];
		}

		auto candidateScores = cgb::make_array<uint32_t>(0u, 0u, 0u, 0u, 0u);
		size_t topScorer = 0;
		
		for (size_t i = 0; i < formatCandidates.size(); ++i) {
			auto formatProps = cgb::context().physical_device().getFormatProperties(formatCandidates[i]);
			candidateScores[i] = static_cast<vk::FormatFeatureFlags::MaskType>(formatProps.optimalTilingFeatures)
							   + static_cast<vk::FormatFeatureFlags::MaskType>(formatProps.linearTilingFeatures)
							   + static_cast<vk::FormatFeatureFlags::MaskType>(formatProps.bufferFeatures);

			if (candidateScores[i] > candidateScores[topScorer]) {
				topScorer = i;
			}
		}
		
		return { formatCandidates[topScorer] };
	}

	image_format image_format::default_depth_stencil_format() noexcept
	{
		const auto formatCandidates = cgb::make_array<vk::Format>(
			vk::Format::eD24UnormS8Uint,
			vk::Format::eD16UnormS8Uint,
			vk::Format::eD32SfloatS8Uint
		);
		
		if (cgb::context().state() < context_state::fully_initialized) {
			return formatCandidates[0];
		}

		auto candidateScores = cgb::make_array<uint32_t>(0u, 0u, 0u);
		size_t topScorer = 0;
		
		for (size_t i = 0; i < formatCandidates.size(); ++i) {
			auto formatProps = cgb::context().physical_device().getFormatProperties(formatCandidates[i]);
			candidateScores[i] = static_cast<vk::FormatFeatureFlags::MaskType>(formatProps.optimalTilingFeatures)
							   + static_cast<vk::FormatFeatureFlags::MaskType>(formatProps.linearTilingFeatures)
							   + static_cast<vk::FormatFeatureFlags::MaskType>(formatProps.bufferFeatures);

			if (candidateScores[i] > candidateScores[topScorer]) {
				topScorer = i;
			}
		}
		
		return { formatCandidates[topScorer] };
	}


	image_format image_format::from_window_color_buffer(window* aWindow)
	{
		if (nullptr == aWindow) {
			aWindow = cgb::context().main_window();
		}
		return aWindow->swap_chain_image_format();
	}
	

	image_format image_format::from_window_depth_buffer(window* aWindow)
	{
		if (nullptr == aWindow) {
			aWindow = cgb::context().main_window();
		}
		for (auto& a : aWindow->get_additional_back_buffer_attachments()) {
			if (a.is_used_as_depth_stencil_attachment()) {
				return a.format();
			}
		}
		return image_format::default_depth_format();
	}
	

	bool is_srgb_format(const image_format& pImageFormat)
	{
		// Note: Currently, the compressed formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> srgbFormats = {
			vk::Format::eR8Srgb,
			vk::Format::eR8G8Srgb,
			vk::Format::eR8G8B8Srgb,
			vk::Format::eB8G8R8Srgb,
			vk::Format::eR8G8B8A8Srgb,
			vk::Format::eB8G8R8A8Srgb,
			vk::Format::eA8B8G8R8SrgbPack32
		};
		auto it = std::find(std::begin(srgbFormats), std::end(srgbFormats), pImageFormat.mFormat);
		return it != srgbFormats.end();
	}

	bool is_uint8_format(const image_format& pImageFormat)
	{
		// Note: Currently, the compressed formats are ignored => could/should be added in the future, maybe
		// TODO: sRGB-formats are assumed to be uint8-formats (not signed int8-formats) => is that true?
		static std::set<vk::Format> uint8Formats = {
			vk::Format::eR8Unorm,
			vk::Format::eR8Uscaled,
			vk::Format::eR8Uint,
			vk::Format::eR8Srgb,
			vk::Format::eR8G8Unorm,
			vk::Format::eR8G8Uscaled,
			vk::Format::eR8G8Uint,
			vk::Format::eR8G8Srgb,
			vk::Format::eR8G8B8Unorm,
			vk::Format::eR8G8B8Uscaled,
			vk::Format::eR8G8B8Uint,
			vk::Format::eR8G8B8Srgb,
			vk::Format::eB8G8R8Unorm,
			vk::Format::eB8G8R8Uscaled,
			vk::Format::eB8G8R8Uint,
			vk::Format::eB8G8R8Srgb,
			vk::Format::eR8G8B8A8Unorm,
			vk::Format::eR8G8B8A8Uscaled,
			vk::Format::eR8G8B8A8Uint,
			vk::Format::eR8G8B8A8Srgb,
			vk::Format::eB8G8R8A8Unorm,
			vk::Format::eB8G8R8A8Uscaled,
			vk::Format::eB8G8R8A8Uint,
			vk::Format::eB8G8R8A8Srgb,
			vk::Format::eA8B8G8R8UnormPack32,
			vk::Format::eA8B8G8R8UscaledPack32,
			vk::Format::eA8B8G8R8UintPack32,
			vk::Format::eA8B8G8R8SrgbPack32
		};
		auto it = std::find(std::begin(uint8Formats), std::end(uint8Formats), pImageFormat.mFormat);
		return it != uint8Formats.end();
	}

	bool is_int8_format(const image_format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> int8Formats = {
			vk::Format::eR8Snorm,
			vk::Format::eR8Sscaled,
			vk::Format::eR8Sint,
			vk::Format::eR8G8Snorm,
			vk::Format::eR8G8Sscaled,
			vk::Format::eR8G8Sint,
			vk::Format::eR8G8B8Snorm,
			vk::Format::eR8G8B8Sscaled,
			vk::Format::eR8G8B8Sint,
			vk::Format::eB8G8R8Snorm,
			vk::Format::eB8G8R8Sscaled,
			vk::Format::eB8G8R8Sint,
			vk::Format::eR8G8B8A8Snorm,
			vk::Format::eR8G8B8A8Sscaled,
			vk::Format::eR8G8B8A8Sint,
			vk::Format::eB8G8R8A8Snorm,
			vk::Format::eB8G8R8A8Sscaled,
			vk::Format::eB8G8R8A8Sint,
			vk::Format::eA8B8G8R8SnormPack32,
			vk::Format::eA8B8G8R8SscaledPack32,
			vk::Format::eA8B8G8R8SintPack32,
		};
		auto it = std::find(std::begin(int8Formats), std::end(int8Formats), pImageFormat.mFormat);
		return it != int8Formats.end();
	}

	bool is_uint16_format(const image_format& pImageFormat)
	{
		// Note: Currently, the compressed formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> uint16Formats = {
			vk::Format::eR16Unorm,
			vk::Format::eR16Uscaled,
			vk::Format::eR16Uint,
			vk::Format::eR16G16Unorm,
			vk::Format::eR16G16Uscaled,
			vk::Format::eR16G16Uint,
			vk::Format::eR16G16B16Unorm,
			vk::Format::eR16G16B16Uscaled,
			vk::Format::eR16G16B16Uint,
			vk::Format::eR16G16B16A16Unorm,
			vk::Format::eR16G16B16A16Uscaled,
			vk::Format::eR16G16B16A16Uint
		};
		auto it = std::find(std::begin(uint16Formats), std::end(uint16Formats), pImageFormat.mFormat);
		return it != uint16Formats.end();
	}

	bool is_int16_format(const image_format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> int16Formats = {
			vk::Format::eR16Snorm,
			vk::Format::eR16Sscaled,
			vk::Format::eR16Sint,
			vk::Format::eR16G16Snorm,
			vk::Format::eR16G16Sscaled,
			vk::Format::eR16G16Sint,
			vk::Format::eR16G16B16Snorm,
			vk::Format::eR16G16B16Sscaled,
			vk::Format::eR16G16B16Sint,
			vk::Format::eR16G16B16A16Snorm,
			vk::Format::eR16G16B16A16Sscaled,
			vk::Format::eR16G16B16A16Sint
		};
		auto it = std::find(std::begin(int16Formats), std::end(int16Formats), pImageFormat.mFormat);
		return it != int16Formats.end();
	}

	bool is_uint32_format(const image_format& pImageFormat)
	{
		// Note: Currently, the compressed formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> uint32Format = { 
			vk::Format::eR32Uint,
			vk::Format::eR32G32Uint,
			vk::Format::eR32G32B32Uint,
			vk::Format::eR32G32B32A32Uint
		};
		auto it = std::find(std::begin(uint32Format), std::end(uint32Format), pImageFormat.mFormat);
		return it != uint32Format.end();
	}

	bool is_int32_format(const image_format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> int32Format = {
			vk::Format::eR32Sint,
			vk::Format::eR32G32Sint,
			vk::Format::eR32G32B32Sint,
			vk::Format::eR32G32B32A32Sint
		};
		auto it = std::find(std::begin(int32Format), std::end(int32Format), pImageFormat.mFormat);
		return it != int32Format.end();
	}

	bool is_float16_format(const image_format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> float16Formats = {
			vk::Format::eR16Sfloat,
			vk::Format::eR16G16Sfloat,
			vk::Format::eR16G16B16Sfloat,
			vk::Format::eR16G16B16A16Sfloat
		};
		auto it = std::find(std::begin(float16Formats), std::end(float16Formats), pImageFormat.mFormat);
		return it != float16Formats.end();
	}

	bool is_float32_format(const image_format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> float32Formats = {
			vk::Format::eR32Sfloat,
			vk::Format::eR32G32Sfloat,
			vk::Format::eR32G32B32Sfloat,
			vk::Format::eR32G32B32A32Sfloat
		};
		auto it = std::find(std::begin(float32Formats), std::end(float32Formats), pImageFormat.mFormat);
		return it != float32Formats.end();
	}

	bool is_float64_format(const image_format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> float64Formats = {
			vk::Format::eR64Sfloat,
			vk::Format::eR64G64Sfloat,
			vk::Format::eR64G64B64Sfloat,
			vk::Format::eR64G64B64A64Sfloat
		};
		auto it = std::find(std::begin(float64Formats), std::end(float64Formats), pImageFormat.mFormat);
		return it != float64Formats.end();
	}

	bool is_rgb_format(const image_format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> rgbFormats = {
			vk::Format::eR5G6B5UnormPack16,
			vk::Format::eR8G8B8Unorm,
			vk::Format::eR8G8B8Snorm,
			vk::Format::eR8G8B8Uscaled,
			vk::Format::eR8G8B8Sscaled,
			vk::Format::eR8G8B8Uint,
			vk::Format::eR8G8B8Sint,
			vk::Format::eR8G8B8Srgb,
			vk::Format::eR16G16B16Unorm,
			vk::Format::eR16G16B16Snorm,
			vk::Format::eR16G16B16Uscaled,
			vk::Format::eR16G16B16Sscaled,
			vk::Format::eR16G16B16Uint,
			vk::Format::eR16G16B16Sint,
			vk::Format::eR16G16B16Sfloat,
			vk::Format::eR32G32B32Uint,
			vk::Format::eR32G32B32Sint,
			vk::Format::eR32G32B32Sfloat,
			vk::Format::eR64G64B64Uint,
			vk::Format::eR64G64B64Sint,
			vk::Format::eR64G64B64Sfloat,

		};
		auto it = std::find(std::begin(rgbFormats), std::end(rgbFormats), pImageFormat.mFormat);
		return it != rgbFormats.end();
	}

	bool is_rgba_format(const image_format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> rgbaFormats = {
			vk::Format::eR4G4B4A4UnormPack16,
			vk::Format::eR5G5B5A1UnormPack16,
			vk::Format::eR8G8B8A8Unorm,
			vk::Format::eR8G8B8A8Snorm,
			vk::Format::eR8G8B8A8Uscaled,
			vk::Format::eR8G8B8A8Sscaled,
			vk::Format::eR8G8B8A8Uint,
			vk::Format::eR8G8B8A8Sint,
			vk::Format::eR8G8B8A8Srgb,
			vk::Format::eR16G16B16A16Unorm,
			vk::Format::eR16G16B16A16Snorm,
			vk::Format::eR16G16B16A16Uscaled,
			vk::Format::eR16G16B16A16Sscaled,
			vk::Format::eR16G16B16A16Uint,
			vk::Format::eR16G16B16A16Sint,
			vk::Format::eR16G16B16A16Sfloat,
			vk::Format::eR32G32B32A32Uint,
			vk::Format::eR32G32B32A32Sint,
			vk::Format::eR32G32B32A32Sfloat,
			vk::Format::eR64G64B64A64Uint,
			vk::Format::eR64G64B64A64Sint,
			vk::Format::eR64G64B64A64Sfloat,
		};
		auto it = std::find(std::begin(rgbaFormats), std::end(rgbaFormats), pImageFormat.mFormat);
		return it != rgbaFormats.end();
	}

	bool is_argb_format(const image_format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> argbFormats = {
			vk::Format::eA1R5G5B5UnormPack16,
			vk::Format::eA2R10G10B10UnormPack32,
			vk::Format::eA2R10G10B10SnormPack32,
			vk::Format::eA2R10G10B10UscaledPack32,
			vk::Format::eA2R10G10B10SscaledPack32,
			vk::Format::eA2R10G10B10UintPack32,
			vk::Format::eA2R10G10B10SintPack32,
		};
		auto it = std::find(std::begin(argbFormats), std::end(argbFormats), pImageFormat.mFormat);
		return it != argbFormats.end();
	}

	bool is_bgr_format(const image_format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> bgrFormats = {
			vk::Format::eB5G6R5UnormPack16,
			vk::Format::eB8G8R8Unorm,
			vk::Format::eB8G8R8Snorm,
			vk::Format::eB8G8R8Uscaled,
			vk::Format::eB8G8R8Sscaled,
			vk::Format::eB8G8R8Uint,
			vk::Format::eB8G8R8Sint,
			vk::Format::eB8G8R8Srgb,
			vk::Format::eB10G11R11UfloatPack32,
		};
		auto it = std::find(std::begin(bgrFormats), std::end(bgrFormats), pImageFormat.mFormat);
		return it != bgrFormats.end();
	}

	bool is_bgra_format(const image_format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> bgraFormats = {
			vk::Format::eB4G4R4A4UnormPack16,
			vk::Format::eB5G5R5A1UnormPack16,
			vk::Format::eR8G8B8A8Unorm,
			vk::Format::eR8G8B8A8Snorm,
			vk::Format::eR8G8B8A8Uscaled,
			vk::Format::eR8G8B8A8Sscaled,
			vk::Format::eR8G8B8A8Uint,
			vk::Format::eR8G8B8A8Sint,
			vk::Format::eR8G8B8A8Srgb,
			vk::Format::eB8G8R8A8Unorm,
			vk::Format::eB8G8R8A8Snorm,
			vk::Format::eB8G8R8A8Uscaled,
			vk::Format::eB8G8R8A8Sscaled,
			vk::Format::eB8G8R8A8Uint,
			vk::Format::eB8G8R8A8Sint,
			vk::Format::eB8G8R8A8Srgb,
		};
		auto it = std::find(std::begin(bgraFormats), std::end(bgraFormats), pImageFormat.mFormat);
		return it != bgraFormats.end();
	}

	bool is_abgr_format(const image_format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> abgrFormats = {
			vk::Format::eA8B8G8R8UnormPack32,
			vk::Format::eA8B8G8R8SnormPack32,
			vk::Format::eA8B8G8R8UscaledPack32,
			vk::Format::eA8B8G8R8SscaledPack32,
			vk::Format::eA8B8G8R8UintPack32,
			vk::Format::eA8B8G8R8SintPack32,
			vk::Format::eA8B8G8R8SrgbPack32,
			vk::Format::eA2B10G10R10UnormPack32,
			vk::Format::eA2B10G10R10SnormPack32,
			vk::Format::eA2B10G10R10UscaledPack32,
			vk::Format::eA2B10G10R10SscaledPack32,
			vk::Format::eA2B10G10R10UintPack32,
			vk::Format::eA2B10G10R10SintPack32,
		};
		auto it = std::find(std::begin(abgrFormats), std::end(abgrFormats), pImageFormat.mFormat);
		return it != abgrFormats.end();
	}

	bool has_stencil_component(const image_format& pImageFormat)
	{
		static std::set<vk::Format> stencilFormats = {
			vk::Format::eD32SfloatS8Uint,
			vk::Format::eD24UnormS8Uint,
		};
		auto it = std::find(std::begin(stencilFormats), std::end(stencilFormats), pImageFormat.mFormat);
		return it != stencilFormats.end();
	}

	bool is_depth_format(const image_format& pImageFormat)
	{
		static std::set<vk::Format> depthFormats = {
			vk::Format::eD16Unorm,
			vk::Format::eD16UnormS8Uint,
			vk::Format::eD24UnormS8Uint,
			vk::Format::eD32Sfloat,
			vk::Format::eD32SfloatS8Uint,
		};
		auto it = std::find(std::begin(depthFormats), std::end(depthFormats), pImageFormat.mFormat);
		return it != depthFormats.end();
	}



	bool is_1component_format(const image_format& pImageFormat)
	{
		static std::set<vk::Format> singleCompFormats = {
			vk::Format::eR8Srgb,
			vk::Format::eR8Unorm,
			vk::Format::eR8Uscaled,
			vk::Format::eR8Uint,
			vk::Format::eR8Srgb,
			vk::Format::eR8Snorm,
			vk::Format::eR8Sscaled,
			vk::Format::eR8Sint,
			vk::Format::eR16Unorm,
			vk::Format::eR16Uscaled,
			vk::Format::eR16Uint,
			vk::Format::eR16Snorm,
			vk::Format::eR16Sscaled,
			vk::Format::eR16Sint,
			vk::Format::eR32Uint,
			vk::Format::eR32Sint,
			vk::Format::eR16Sfloat,
			vk::Format::eR32Sfloat,
			vk::Format::eR64Sfloat,
		};
		auto it = std::find(std::begin(singleCompFormats), std::end(singleCompFormats), pImageFormat.mFormat);
		return it != singleCompFormats.end();
	}

	bool is_2component_format(const image_format& pImageFormat)
	{
		static std::set<vk::Format> twoComponentFormats = {
			vk::Format::eR8G8Srgb,
			vk::Format::eR8G8Unorm,
			vk::Format::eR8G8Uscaled,
			vk::Format::eR8G8Uint,
			vk::Format::eR8G8Srgb,
			vk::Format::eR8G8Snorm,
			vk::Format::eR8G8Sscaled,
			vk::Format::eR8G8Sint,
			vk::Format::eR16G16Unorm,
			vk::Format::eR16G16Uscaled,
			vk::Format::eR16G16Uint,
			vk::Format::eR16G16Snorm,
			vk::Format::eR16G16Sscaled,
			vk::Format::eR16G16Sint,
			vk::Format::eR32G32Uint,
			vk::Format::eR32G32Sint,
			vk::Format::eR16G16Sfloat,
			vk::Format::eR32G32Sfloat,
			vk::Format::eR64G64Sfloat,
		};
		auto it = std::find(std::begin(twoComponentFormats), std::end(twoComponentFormats), pImageFormat.mFormat);
		return it != twoComponentFormats.end();
	}

	bool is_3component_format(const image_format& pImageFormat)
	{
		static std::set<vk::Format> threeCompFormat = {
			vk::Format::eR8G8B8Srgb,
			vk::Format::eB8G8R8Srgb,
			vk::Format::eR5G6B5UnormPack16,
			vk::Format::eR8G8B8Unorm,
			vk::Format::eR8G8B8Snorm,
			vk::Format::eR8G8B8Uscaled,
			vk::Format::eR8G8B8Sscaled,
			vk::Format::eR8G8B8Uint,
			vk::Format::eR8G8B8Sint,
			vk::Format::eR8G8B8Srgb,
			vk::Format::eR16G16B16Unorm,
			vk::Format::eR16G16B16Snorm,
			vk::Format::eR16G16B16Uscaled,
			vk::Format::eR16G16B16Sscaled,
			vk::Format::eR16G16B16Uint,
			vk::Format::eR16G16B16Sint,
			vk::Format::eR16G16B16Sfloat,
			vk::Format::eR32G32B32Uint,
			vk::Format::eR32G32B32Sint,
			vk::Format::eR32G32B32Sfloat,
			vk::Format::eR64G64B64Uint,
			vk::Format::eR64G64B64Sint,
			vk::Format::eR64G64B64Sfloat,
			vk::Format::eB5G6R5UnormPack16,
			vk::Format::eB8G8R8Unorm,
			vk::Format::eB8G8R8Snorm,
			vk::Format::eB8G8R8Uscaled,
			vk::Format::eB8G8R8Sscaled,
			vk::Format::eB8G8R8Uint,
			vk::Format::eB8G8R8Sint,
			vk::Format::eB8G8R8Srgb,
			vk::Format::eB10G11R11UfloatPack32,
		};
		auto it = std::find(std::begin(threeCompFormat), std::end(threeCompFormat), pImageFormat.mFormat);
		return it != threeCompFormat.end();
	}

	bool is_4component_format(const image_format& pImageFormat)
	{
		static std::set<vk::Format> fourCompFormats = {
			vk::Format::eR8G8B8A8Srgb,
			vk::Format::eB8G8R8A8Srgb,
			vk::Format::eA8B8G8R8SrgbPack32,
			vk::Format::eR4G4B4A4UnormPack16,
			vk::Format::eR5G5B5A1UnormPack16,
			vk::Format::eR8G8B8A8Unorm,
			vk::Format::eR8G8B8A8Snorm,
			vk::Format::eR8G8B8A8Uscaled,
			vk::Format::eR8G8B8A8Sscaled,
			vk::Format::eR8G8B8A8Uint,
			vk::Format::eR8G8B8A8Sint,
			vk::Format::eR8G8B8A8Srgb,
			vk::Format::eR16G16B16A16Unorm,
			vk::Format::eR16G16B16A16Snorm,
			vk::Format::eR16G16B16A16Uscaled,
			vk::Format::eR16G16B16A16Sscaled,
			vk::Format::eR16G16B16A16Uint,
			vk::Format::eR16G16B16A16Sint,
			vk::Format::eR16G16B16A16Sfloat,
			vk::Format::eR32G32B32A32Uint,
			vk::Format::eR32G32B32A32Sint,
			vk::Format::eR32G32B32A32Sfloat,
			vk::Format::eR64G64B64A64Uint,
			vk::Format::eR64G64B64A64Sint,
			vk::Format::eR64G64B64A64Sfloat,
			vk::Format::eA1R5G5B5UnormPack16,
			vk::Format::eA2R10G10B10UnormPack32,
			vk::Format::eA2R10G10B10SnormPack32,
			vk::Format::eA2R10G10B10UscaledPack32,
			vk::Format::eA2R10G10B10SscaledPack32,
			vk::Format::eA2R10G10B10UintPack32,
			vk::Format::eA2R10G10B10SintPack32,
			vk::Format::eB4G4R4A4UnormPack16,
			vk::Format::eB5G5R5A1UnormPack16,
			vk::Format::eR8G8B8A8Unorm,
			vk::Format::eR8G8B8A8Snorm,
			vk::Format::eR8G8B8A8Uscaled,
			vk::Format::eR8G8B8A8Sscaled,
			vk::Format::eR8G8B8A8Uint,
			vk::Format::eR8G8B8A8Sint,
			vk::Format::eR8G8B8A8Srgb,
			vk::Format::eB8G8R8A8Unorm,
			vk::Format::eB8G8R8A8Snorm,
			vk::Format::eB8G8R8A8Uscaled,
			vk::Format::eB8G8R8A8Sscaled,
			vk::Format::eB8G8R8A8Uint,
			vk::Format::eB8G8R8A8Sint,
			vk::Format::eB8G8R8A8Srgb,
			vk::Format::eA8B8G8R8UnormPack32,
			vk::Format::eA8B8G8R8SnormPack32,
			vk::Format::eA8B8G8R8UscaledPack32,
			vk::Format::eA8B8G8R8SscaledPack32,
			vk::Format::eA8B8G8R8UintPack32,
			vk::Format::eA8B8G8R8SintPack32,
			vk::Format::eA8B8G8R8SrgbPack32,
			vk::Format::eA2B10G10R10UnormPack32,
			vk::Format::eA2B10G10R10SnormPack32,
			vk::Format::eA2B10G10R10UscaledPack32,
			vk::Format::eA2B10G10R10SscaledPack32,
			vk::Format::eA2B10G10R10UintPack32,
			vk::Format::eA2B10G10R10SintPack32,
		};
		auto it = std::find(std::begin(fourCompFormats), std::end(fourCompFormats), pImageFormat.mFormat);
		return it != fourCompFormats.end();
	}






	std::tuple<vk::ImageUsageFlags, vk::ImageLayout, vk::ImageTiling, vk::ImageCreateFlags> determine_usage_layout_tiling_flags_based_on_image_usage(image_usage _ImageUsageFlags)
	{
		vk::ImageUsageFlags imageUsage{};

		bool isReadOnly = (_ImageUsageFlags & image_usage::read_only) == image_usage::read_only;
		image_usage cleanedUpUsageFlagsForReadOnly = exclude(_ImageUsageFlags, image_usage::transfer_source | image_usage::transfer_destination | image_usage::sampled | image_usage::read_only | image_usage::presentable | image_usage::shared_presentable | image_usage::tiling_optimal | image_usage::tiling_linear | image_usage::sparse_memory_binding | image_usage::cube_compatible | image_usage::is_protected); // TODO: To be verified, it's just a guess.

		auto targetLayout = isReadOnly ? vk::ImageLayout::eShaderReadOnlyOptimal : vk::ImageLayout::eGeneral; // General Layout or Shader Read Only Layout is the default
		auto imageTiling = vk::ImageTiling::eOptimal; // Optimal is the default
		vk::ImageCreateFlags imageCreateFlags{};

		if ((_ImageUsageFlags & image_usage::transfer_source) == image_usage::transfer_source) {
			imageUsage |= vk::ImageUsageFlagBits::eTransferSrc;
			image_usage cleanedUpUsageFlags = exclude(_ImageUsageFlags, image_usage::read_only | image_usage::presentable | image_usage::shared_presentable | image_usage::tiling_optimal | image_usage::tiling_linear | image_usage::sparse_memory_binding | image_usage::cube_compatible | image_usage::is_protected); // TODO: To be verified, it's just a guess.
			if (image_usage::transfer_source == cleanedUpUsageFlags) {
				targetLayout = vk::ImageLayout::eTransferSrcOptimal;
			}
			else {
				targetLayout = vk::ImageLayout::eGeneral;
			}
		}
		if ((_ImageUsageFlags & image_usage::transfer_destination) == image_usage::transfer_destination) {
			imageUsage |= vk::ImageUsageFlagBits::eTransferDst;
			image_usage cleanedUpUsageFlags = exclude(_ImageUsageFlags, image_usage::read_only | image_usage::presentable | image_usage::shared_presentable | image_usage::tiling_optimal | image_usage::tiling_linear | image_usage::sparse_memory_binding | image_usage::cube_compatible | image_usage::is_protected); // TODO: To be verified, it's just a guess.
			if (image_usage::transfer_destination == cleanedUpUsageFlags) {
				targetLayout = vk::ImageLayout::eTransferDstOptimal;
			}
			else {
				targetLayout = vk::ImageLayout::eGeneral;
			}
		}
		if ((_ImageUsageFlags & image_usage::sampled) == image_usage::sampled) {
			imageUsage |= vk::ImageUsageFlagBits::eSampled;
		}
		if ((_ImageUsageFlags & image_usage::color_attachment) == image_usage::color_attachment) {
			imageUsage |= vk::ImageUsageFlagBits::eColorAttachment;
			targetLayout = vk::ImageLayout::eColorAttachmentOptimal;
		}
		if ((_ImageUsageFlags & image_usage::depth_stencil_attachment) == image_usage::depth_stencil_attachment) {
			imageUsage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
			if (isReadOnly && image_usage::depth_stencil_attachment == cleanedUpUsageFlagsForReadOnly) {
				targetLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal;
			}
			else {
				targetLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
			}
		}
		if ((_ImageUsageFlags & image_usage::input_attachment) == image_usage::input_attachment) {
			imageUsage |= vk::ImageUsageFlagBits::eInputAttachment;
		}
		if ((_ImageUsageFlags & image_usage::shading_rate_image) == image_usage::shading_rate_image) {
			imageUsage |= vk::ImageUsageFlagBits::eShadingRateImageNV;
		}
		if ((_ImageUsageFlags & image_usage::presentable) == image_usage::presentable) {
			targetLayout = vk::ImageLayout::ePresentSrcKHR; // TODO: This probably needs some further action(s) => implement that further action(s)
		}
		if ((_ImageUsageFlags & image_usage::shared_presentable) == image_usage::shared_presentable) {
			targetLayout = vk::ImageLayout::eSharedPresentKHR; // TODO: This probably needs some further action(s) => implement that further action(s)
		}
		if ((_ImageUsageFlags & image_usage::tiling_optimal) == image_usage::tiling_optimal) {
			imageTiling = vk::ImageTiling::eOptimal;
		}
		if ((_ImageUsageFlags & image_usage::tiling_linear) == image_usage::tiling_linear) {
			imageTiling = vk::ImageTiling::eLinear;
		}
		if ((_ImageUsageFlags & image_usage::sparse_memory_binding) == image_usage::sparse_memory_binding) {
			imageCreateFlags |= vk::ImageCreateFlagBits::eSparseBinding;
		}
		if ((_ImageUsageFlags & image_usage::cube_compatible) == image_usage::cube_compatible) {
			imageCreateFlags |= vk::ImageCreateFlagBits::eCubeCompatible;
		}
		if ((_ImageUsageFlags & image_usage::is_protected) == image_usage::is_protected) {
			imageCreateFlags |= vk::ImageCreateFlagBits::eProtected;
		}
		if ((_ImageUsageFlags & image_usage::shader_storage) == image_usage::shader_storage) { 
			imageUsage |= vk::ImageUsageFlagBits::eStorage;	
			// Can not be Shader Read Only Layout
			targetLayout = vk::ImageLayout::eGeneral; // TODO: Verify that this should always be in general layout!
		}

		return std::make_tuple(imageUsage, targetLayout, imageTiling, imageCreateFlags);
	}

	image_t::image_t(const image_t& aOther)
	{
		if (std::holds_alternative<vk::Image>(aOther.mImage)) {
			assert(!aOther.mMemory);
			mInfo = aOther.mInfo; 
			mImage = std::get<vk::Image>(aOther.mImage);
			mTargetLayout = aOther.mTargetLayout;
			mCurrentLayout = aOther.mCurrentLayout;
			mImageUsage = aOther.mImageUsage;
			mAspectFlags = aOther.mAspectFlags;
		}
		else {
			throw cgb::runtime_error("Can not copy this image instance!");
		}
	}
	
	owning_resource<image_t> image_t::create(uint32_t aWidth, uint32_t aHeight, image_format aFormat, bool aUseMipMaps, int aNumLayers, memory_usage aMemoryUsage, image_usage aImageUsage, context_specific_function<void(image_t&)> aAlterConfigBeforeCreation)
	{
		// Determine image usage flags, image layout, and memory usage flags:
		auto [imageUsage, targetLayout, imageTiling, imageCreateFlags] = determine_usage_layout_tiling_flags_based_on_image_usage(aImageUsage);

		vk::MemoryPropertyFlags memoryFlags{};
		switch (aMemoryUsage) {
		case cgb::memory_usage::host_visible:
			memoryFlags = vk::MemoryPropertyFlagBits::eHostVisible;
			break;
		case cgb::memory_usage::host_coherent:
			memoryFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
			break;
		case cgb::memory_usage::host_cached:
			memoryFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCached;
			break;
		case cgb::memory_usage::device:
			memoryFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;
			imageUsage |= vk::ImageUsageFlagBits::eTransferDst; 
			break;
		case cgb::memory_usage::device_readback:
			memoryFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;
			imageUsage |= vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc;
			break;
		case cgb::memory_usage::device_protected:
			memoryFlags = vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eProtected;
			imageUsage |= vk::ImageUsageFlagBits::eTransferDst;
			break;
		}

		// How many MIP-map levels are we going to use?
		auto mipLevels = aUseMipMaps
			? static_cast<uint32_t>(std::floor(std::log2(std::max(aWidth, aHeight))) + 1)
			: 1u;

		vk::ImageAspectFlags aspectFlags = {};
		if (is_depth_format(aFormat)) {
			aspectFlags |= vk::ImageAspectFlagBits::eDepth;
		}
		if (has_stencil_component(aFormat)) {
			aspectFlags |= vk::ImageAspectFlagBits::eStencil;
		}
		if (!aspectFlags) {
			aspectFlags = vk::ImageAspectFlagBits::eColor;
			// TODO: maybe support further aspect flags?!
		}

		image_t result;
		result.mAspectFlags = aspectFlags;
		result.mCurrentLayout = vk::ImageLayout::eUndefined;
		result.mTargetLayout = targetLayout;
		result.mInfo = vk::ImageCreateInfo()
			.setImageType(vk::ImageType::e2D) // TODO: Support 3D textures
			.setExtent(vk::Extent3D(static_cast<uint32_t>(aWidth), static_cast<uint32_t>(aHeight), 1u))
			.setMipLevels(mipLevels)
			.setArrayLayers(1u) // TODO: support multiple array layers
			.setFormat(aFormat.mFormat)
			.setTiling(imageTiling)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setUsage(imageUsage)
			.setSharingMode(vk::SharingMode::eExclusive) // TODO: Not sure yet how to handle this one, Exclusive should be the default, though.
			.setSamples(vk::SampleCountFlagBits::e1)
			.setFlags(imageCreateFlags); // Optional;

		// Maybe alter the config?!
		if (aAlterConfigBeforeCreation.mFunction) {
			aAlterConfigBeforeCreation.mFunction(result);
		}

		// Create the image...
		result.mImage = context().logical_device().createImageUnique(result.mInfo);

		// ... and the memory:
		auto memRequirements = context().logical_device().getImageMemoryRequirements(result.handle());
		auto allocInfo = vk::MemoryAllocateInfo()
			.setAllocationSize(memRequirements.size)
			.setMemoryTypeIndex(context().find_memory_type_index(memRequirements.memoryTypeBits, memoryFlags));
		result.mMemory = context().logical_device().allocateMemoryUnique(allocInfo);

		// bind them together:
		context().logical_device().bindImageMemory(result.handle(), result.memory_handle(), 0);
		
		return result;
	}

	owning_resource<image_t> image_t::create_depth(uint32_t pWidth, uint32_t pHeight, std::optional<image_format> pFormat, bool pUseMipMaps, int pNumLayers,  memory_usage pMemoryUsage, image_usage pImageUsage, context_specific_function<void(image_t&)> pAlterConfigBeforeCreation)
	{
		// Select a suitable depth format
		if (!pFormat) {
			std::array depthFormats = { vk::Format::eD32Sfloat, vk::Format::eD24UnormS8Uint, vk::Format::eD16Unorm };
			for (auto format : depthFormats) {
				if (cgb::context().is_format_supported(format, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment)) {
					pFormat = format;
					break;
				}
			}
		}
		if (!pFormat) {
			throw cgb::runtime_error("No suitable depth format could be found.");
		}

		pImageUsage |= image_usage::depth_stencil_attachment;

		// Create the image (by default only on the device which should be sufficient for a depth buffer => see pMemoryUsage's default value):
		auto result = image_t::create(pWidth, pHeight, *pFormat, pUseMipMaps, pNumLayers, pMemoryUsage, pImageUsage, std::move(pAlterConfigBeforeCreation));
		result->mAspectFlags = vk::ImageAspectFlagBits::eDepth;
		return result;
	}

	owning_resource<image_t> image_t::create_depth_stencil(uint32_t pWidth, uint32_t pHeight, std::optional<image_format> pFormat, bool pUseMipMaps, int pNumLayers,  memory_usage pMemoryUsage, image_usage pImageUsage, context_specific_function<void(image_t&)> pAlterConfigBeforeCreation)
	{
		// Select a suitable depth+stencil format
		if (!pFormat) {
			std::array depthFormats = { vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint, vk::Format::eD16UnormS8Uint };
			for (auto format : depthFormats) {
				if (cgb::context().is_format_supported(format, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment)) {
					pFormat = format;
					break;
				}
			}
		}
		if (!pFormat) {
			throw cgb::runtime_error("No suitable depth+stencil format could be found.");
		}

		// Create the image (by default only on the device which should be sufficient for a depth+stencil buffer => see pMemoryUsage's default value):
		auto result = image_t::create_depth(pWidth, pHeight, *pFormat, pUseMipMaps, pNumLayers, pMemoryUsage, pImageUsage, std::move(pAlterConfigBeforeCreation));
		result->mAspectFlags = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
		return result;
	}

	image_t image_t::wrap(vk::Image aImageToWrap, vk::ImageCreateInfo aImageCreateInfo, image_usage aImageUsage, vk::ImageAspectFlags aImageAspectFlags)
	{
		auto [imageUsage, targetLayout, imageTiling, imageCreateFlags] = determine_usage_layout_tiling_flags_based_on_image_usage(aImageUsage);
		
		image_t result;
		result.mInfo = aImageCreateInfo;
		result.mImage = aImageToWrap;		
		result.mTargetLayout = targetLayout;
		result.mCurrentLayout = vk::ImageLayout::eUndefined;
		result.mImageUsage = aImageUsage;
		result.mAspectFlags = vk::ImageAspectFlagBits::eColor;
		return result;
	}
	
	vk::ImageSubresourceRange image_t::entire_subresource_range() const
	{
		return vk::ImageSubresourceRange{
			mAspectFlags,
			0u, mInfo.mipLevels,	// MIP info
			0u, mInfo.arrayLayers	// Layers info
		};
	}


	std::optional<command_buffer> image_t::transition_to_layout(std::optional<vk::ImageLayout> aTargetLayout, sync aSyncHandler)
	{
		const auto curLayout = current_layout();
		const auto trgLayout = aTargetLayout.value_or(target_layout());
		mTargetLayout = trgLayout;

		if (curLayout == trgLayout) {
			return {}; // done (:
		}
		if (vk::ImageLayout::eUndefined == trgLayout || vk::ImageLayout::ePreinitialized == trgLayout) {
			LOG_VERBOSE(fmt::format("Won't transition into layout {}.", to_string(trgLayout)));
			return {}; // Won't do it!
		}
		
		aSyncHandler.set_queue_hint(cgb::context().transfer_queue());

		// Not done => perform a transition via an image memory barrier inside a command buffer
		auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
		aSyncHandler.establish_barrier_before_the_operation(
			pipeline_stage::transfer,	// Just use the transfer stage to create an execution dependency chain
			read_memory_access{memory_access::transfer_read_access}
		);

		// An image's layout is tranformed by the means of an image memory barrier:
		commandBuffer.establish_image_memory_barrier(*this,
			pipeline_stage::transfer, pipeline_stage::transfer,				// Execution dependency chain
			std::optional<memory_access>{}, std::optional<memory_access>{}	// There should be no need to make any memory available or visible... the image should be available already (see above)
		); // establish_image_memory_barrier ^ will set the mCurrentLayout to mTargetLayout

		aSyncHandler.establish_barrier_after_the_operation(
			pipeline_stage::transfer,	// The end of the execution dependency chain
			write_memory_access{memory_access::transfer_write_access}
		);
		return aSyncHandler.submit_and_sync();
	}


}
