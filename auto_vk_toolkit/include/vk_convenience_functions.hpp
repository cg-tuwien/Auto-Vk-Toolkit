#pragma once

namespace avk
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
	vk::Format format_from_window_color_buffer(window* aWindow);
	vk::Format format_from_window_depth_buffer(window* aWindow);
	vk::Extent3D for_each_pixel(window* aWindow);
	glm::mat4 to_mat(const VkTransformMatrixKHR& aRowMajor3x4Matrix);
}
