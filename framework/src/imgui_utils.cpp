#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_utils.h"
#include "imgui_internal.h"

void ImGui::ImageWithBg(ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col, const ImVec4& bg_col)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
    if (border_col.w > 0.0f)
        bb.Max += ImVec2(2, 2);
    ItemSize(bb);
    if (!ItemAdd(bb, 0))
        return;

	if (bg_col.w > 0.0f) // background
	{
		window->DrawList->AddRectFilled(bb.Min, bb.Max, GetColorU32(bg_col), 0.0f);
	}

    if (border_col.w > 0.0f) // border
	{
		window->DrawList->AddRect(bb.Min, bb.Max, GetColorU32(border_col), 0.0f);
		// add image acounting for border
		window->DrawList->AddImage(user_texture_id, bb.Min + ImVec2(1, 1), bb.Max - ImVec2(1, 1), uv0, uv1, GetColorU32(tint_col));
	}
    else // no border
    {
		// add image as is
        window->DrawList->AddImage(user_texture_id, bb.Min, bb.Max, uv0, uv1, GetColorU32(tint_col));
    }
}

void ImGui::ImageStack(std::initializer_list<ImTextureID> user_texture_ids, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col, const ImVec4& bg_col)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
    if (border_col.w > 0.0f)
        bb.Max += ImVec2(2, 2);
    ItemSize(bb);
    if (!ItemAdd(bb, 0))
        return;

	if (bg_col.w > 0.0f) // background
	{
		window->DrawList->AddRectFilled(bb.Min, bb.Max, GetColorU32(bg_col), 0.0f);
	}

    if (border_col.w > 0.0f) // border
	{
		window->DrawList->AddRect(bb.Min, bb.Max, GetColorU32(border_col), 0.0f);
		// add images acounting for border
        for (auto user_texture_id : user_texture_ids)
        {
			window->DrawList->AddImage(user_texture_id, bb.Min + ImVec2(1, 1), bb.Max - ImVec2(1, 1), uv0, uv1, GetColorU32(tint_col));
        }
	}
    else // no border
    {
		// add images as is
        for (auto user_texture_id : user_texture_ids)
        {
			window->DrawList->AddImage(user_texture_id, bb.Min, bb.Max, uv0, uv1, GetColorU32(tint_col));
        }
    }
}
