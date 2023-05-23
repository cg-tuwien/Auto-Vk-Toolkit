#pragma once

#include <initializer_list>
#include <limits>

#include "imgui.h"

namespace ImGui
{
	/** @brief Draw an image with a background
	 *
	 *	This function adds an image with a Background to the draw list. The background is only visible if the image has opaque part.
	 */
    IMGUI_API void ImageWithBg(ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0), const ImVec4& bg_col = ImVec4(0, 0, 0, 0));

	/** @brief Draw images on top of each other
	 *
	 *	This function adds multiple images on top of each other to the draw list. The images are added in order provided through the initializer_list,
	 *	i.e. the first image in the list is rendered first and the last image is rendered last.
	 */
    IMGUI_API void ImageStack(std::initializer_list<ImTextureID> user_texture_ids, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0), const ImVec4& bg_col = ImVec4(0, 0, 0, 0));
}
