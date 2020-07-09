#include "vk_convenience_functions.hpp"

namespace xk
{
	vk::Format default_rgb8_4comp_format() noexcept
	{
		return vk::Format::eR8G8B8A8Unorm;
	}
	
	vk::Format default_rgb8_3comp_format() noexcept
	{
		return vk::Format::eR8G8B8Unorm;
	}
	
	vk::Format default_rgb8_2comp_format() noexcept
	{
		return vk::Format::eR8G8Unorm;
	}
	
	vk::Format default_rgb8_1comp_format() noexcept
	{
		return vk::Format::eR8Unorm;
	}
	
	vk::Format default_srgb_4comp_format() noexcept
	{
		return vk::Format::eR8G8B8A8Srgb;
	}
	
	vk::Format default_srgb_3comp_format() noexcept
	{
		return vk::Format::eR8G8B8Srgb;
	}
	
	vk::Format default_srgb_2comp_format() noexcept
	{
		return vk::Format::eR8G8Srgb;
	}
	
	vk::Format default_srgb_1comp_format() noexcept
	{
		return vk::Format::eR8Srgb;
	}
	
	vk::Format default_rgb16f_4comp_format() noexcept
	{
		return vk::Format::eR16G16B16A16Sfloat;
	}
	
	vk::Format default_rgb16f_3comp_format() noexcept
	{
		return vk::Format::eR16G16B16Sfloat;
	}
	
	vk::Format default_rgb16f_2comp_format() noexcept
	{
		return vk::Format::eR16G16Sfloat;
	}
	
	vk::Format default_rgb16f_1comp_format() noexcept
	{
		return vk::Format::eR16Sfloat;
	}
	

	vk::Format default_depth_format() noexcept
	{
		const auto formatCandidates = ak::make_array<vk::Format>(
			vk::Format::eD32Sfloat,
			vk::Format::eD24UnormS8Uint,
			vk::Format::eD16Unorm,
			vk::Format::eD16UnormS8Uint,
			vk::Format::eD32SfloatS8Uint
		);

		if (xk::context().state() < context_state::fully_initialized) {
			return formatCandidates[0];
		}

		auto candidateScores = ak::make_array<uint32_t>(0u, 0u, 0u, 0u, 0u);
		size_t topScorer = 0;
		
		for (size_t i = 0; i < formatCandidates.size(); ++i) {
			auto formatProps = xk::context().physical_device().getFormatProperties(formatCandidates[i]);
			candidateScores[i] = static_cast<uint32_t>(formatProps.optimalTilingFeatures)
							   + static_cast<uint32_t>(formatProps.linearTilingFeatures)
							   + static_cast<uint32_t>(formatProps.bufferFeatures);

			if (candidateScores[i] > candidateScores[topScorer]) {
				topScorer = i;
			}
		}
		
		return formatCandidates[topScorer];
	}

	vk::Format default_depth_stencil_format() noexcept
	{
		const auto formatCandidates = ak::make_array<vk::Format>(
			vk::Format::eD24UnormS8Uint,
			vk::Format::eD16UnormS8Uint,
			vk::Format::eD32SfloatS8Uint
		);
		
		if (xk::context().state() < context_state::fully_initialized) {
			return formatCandidates[0];
		}

		auto candidateScores = ak::make_array<uint32_t>(0u, 0u, 0u);
		size_t topScorer = 0;
		
		for (size_t i = 0; i < formatCandidates.size(); ++i) {
			auto formatProps = xk::context().physical_device().getFormatProperties(formatCandidates[i]);
			candidateScores[i] = static_cast<uint32_t>(formatProps.optimalTilingFeatures)
							   + static_cast<uint32_t>(formatProps.linearTilingFeatures)
							   + static_cast<uint32_t>(formatProps.bufferFeatures);

			if (candidateScores[i] > candidateScores[topScorer]) {
				topScorer = i;
			}
		}
		
		return formatCandidates[topScorer];
	}


	vk::Format from_window_color_buffer(window* aWindow)
	{
		if (nullptr == aWindow) {
			aWindow = xk::context().main_window();
		}
		return aWindow->swap_chain_image_format();
	}
	

	vk::Format from_window_depth_buffer(window* aWindow)
	{
		if (nullptr == aWindow) {
			aWindow = xk::context().main_window();
		}
		for (auto& a : aWindow->get_additional_back_buffer_attachments()) {
			if (a.is_used_as_depth_stencil_attachment()) {
				return a.format();
			}
		}
		return default_depth_format();
	}
}
