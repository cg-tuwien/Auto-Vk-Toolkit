#pragma once

namespace cgb
{
	vk::Format default_rgb8_4comp_format() noexcept;
	vk::Format default_rgb8_3comp_format() noexcept;
	vk::Format default_rgb8_2comp_format() noexcept;
	vk::Format default_rgb8_1comp_format() noexcept;
	vk::Format default_srgb_4comp_format() noexcept;
	vk::Format default_srgb_3comp_format() noexcept;
	vk::Format default_srgb_2comp_format() noexcept;
	vk::Format default_srgb_1comp_format() noexcept;
	vk::Format default_rgb16f_4comp_format() noexcept;
	vk::Format default_rgb16f_3comp_format() noexcept;
	vk::Format default_rgb16f_2comp_format() noexcept;
	vk::Format default_rgb16f_1comp_format() noexcept;
	vk::Format default_depth_format() noexcept;
	vk::Format default_depth_stencil_format() noexcept;
	vk::Format from_window_color_buffer(window* aWindow);
}
