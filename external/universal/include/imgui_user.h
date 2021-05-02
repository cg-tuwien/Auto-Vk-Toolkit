#include <limits>
#include <imgui.h>

namespace ImGui
{
    void Image(ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col, const ImVec4& bg_col);
	void StackedImage(std::initializer_list<ImTextureID> user_texture_ids, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col, const ImVec4& bg_col);
}
