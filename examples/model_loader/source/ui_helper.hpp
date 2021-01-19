#include <gvk.hpp>
#include <imgui.h>
#include <any>

/** ImGui for some reason saves its arithmatic types into some enum - determine the type
*/
class imgui_data_type_determinator
{
	static const std::unordered_map<std::type_index, ImGuiDataType> imguiDataTypesMap;
public:
	static ImGuiDataType get_type_for(std::type_index aCppType)
	{
		return imguiDataTypesMap.at(aCppType);
	}
	static ImGuiDataType get_type_for(std::any aValue)
	{
		return imguiDataTypesMap.at(std::type_index(aValue.type()));
	}
};
const std::unordered_map<std::type_index, ImGuiDataType> imgui_data_type_determinator::imguiDataTypesMap = {
	{ std::type_index(typeid(signed char)),     ImGuiDataType_S8    },
	{ std::type_index(typeid(unsigned char)),   ImGuiDataType_U8    },
	{ std::type_index(typeid(short)),           ImGuiDataType_S16   },
	{ std::type_index(typeid(unsigned short)),  ImGuiDataType_U16   },
	{ std::type_index(typeid(int)),             ImGuiDataType_S32   },
	{ std::type_index(typeid(unsigned int)),    ImGuiDataType_U32   },
	{ std::type_index(typeid(uint64_t)),        ImGuiDataType_U64   },
	{ std::type_index(typeid(int64_t)),         ImGuiDataType_S64   },
	{ std::type_index(typeid(float)),           ImGuiDataType_Float },
	{ std::type_index(typeid(double)),          ImGuiDataType_Double},
};

/** structure used to setup an ImGui slider with callback
*/
template <typename T> requires std::integral<T> || std::floating_point<T>
class slider_container {
	std::string mTitle;
	std::optional<std::function<void(T)>> mCallback;
	T mCurrent;
	T mMin;
	T mMax;
	ImGuiDataType mDataType;
public:
	slider_container(std::string&& title, T current, T min, T max, std::optional<std::function<void(T)>>&& callback)
		: mTitle(title), mCurrent(current), mMin(min), mMax(max), mCallback(std::move(callback))
	{
		mDataType = imgui_data_type_determinator::get_type_for(std::type_index(typeid(T)));
	};

	void invokeImGui()
	{
		if (ImGui::SliderScalar(mTitle.c_str(), mDataType, &mCurrent, &mMin, &mMax)) {
			if (mCallback.has_value()) {
				mCallback.value()(mCurrent);
			}
		}
	}
};

/** structure used to setup an ImGui Combo with callback
*/
class combo_box_container {
	const static int default_max_visible_count = 6;
	int mCurrent;
	std::string mTitle;
	std::vector<const char*> mItems;
	std::optional<std::function<void(std::string)>> mCallback;
	int mVisibleRowsCount;
public:
	combo_box_container(std::string&& aTitle, std::vector<const char*>&& aItems, int aCurrent, std::optional<std::function<void(std::string)>>&& aCallback)
		: mTitle(aTitle), mItems(aItems), mCurrent(aCurrent), mCallback(std::move(aCallback)) {
		mVisibleRowsCount = std::min(static_cast<size_t>(default_max_visible_count), mItems.size());
	};

	void invokeImGui()
	{
		if (ImGui::Combo(mTitle.c_str(), &mCurrent, mItems.data(), mVisibleRowsCount)) {
			if (mCallback.has_value()) {
				mCallback.value()(std::string(mItems[mCurrent]));
			}
		}
	}
};

/** structure used to setup an ImGui Checkbox with callback
*/
class check_box_container {

	std::string mTitle;
	bool mIsChecked = false;
	std::optional<std::function<void(bool)>> mCallback;
public:
	check_box_container(std::string&& title, bool initialValue, std::optional<std::function<void(bool)>>&& callback)
		: mTitle(title), mIsChecked(initialValue), mCallback(std::move(callback)) {};

	void invokeImGui()
	{
		if (ImGui::Checkbox(mTitle.c_str(), &mIsChecked)) {
			if (mCallback.has_value()) {
				mCallback.value()(checked());
			}
		}
	}
	bool checked() const { return mIsChecked; }
};

// model_loader helper functions to reduce code clutter
class model_loader_ui_generator {
public:
	static combo_box_container get_presentation_mode_imgui_element()
	{
		std::vector<const char*> items { "immediate", "fifo", "mailbox", "relaxed_fifo" };

		std::unordered_map<std::string, gvk::presentation_mode> stringMap = {
			{items[0],	gvk::presentation_mode::immediate},
			{items[1],	gvk::presentation_mode::fifo},
			{items[2],	gvk::presentation_mode::mailbox},
			{items[3],	gvk::presentation_mode::relaxed_fifo}
		};

		std::function<void(std::string)> cb = [stringMap](const std::string aItem) {
			gvk::context().main_window()->set_presentaton_mode(stringMap.at(aItem));
		};

		return std::move(combo_box_container{"Presentation Mode", std::move(items), 0, std::move(cb) });
	}

	static check_box_container get_framebuffer_mode_imgui_element()
	{
		std::function<void(bool)> cb = [](bool aRequestSrgb) {
			gvk::context().main_window()->request_srgb_framebuffer(aRequestSrgb);
		};

		return std::move(check_box_container{ "SRGB framebuffer", false, std::move(cb) });
	}

	static check_box_container get_window_resize_imgui_element()
	{
		std::function<void(bool)> cb = [](bool aResizable) {
			gvk::context().main_window()->enable_resizing(aResizable);
		};

		return std::move(check_box_container{ "Resizable Window", true, std::move(cb) });
	}

	static slider_container<int> get_number_of_concurrent_frames_imgui_element(int aCurrent = 3, int aMin = 1, int aMax = 10)
	{
		std::function<void(int)> cb = [](int aNumConcurrentFrames) {
			gvk::context().main_window()->set_number_of_concurrent_frames(aNumConcurrentFrames);
		};
		assert(aCurrent >= aMin);
		assert(aMax >= aCurrent);
		return std::move(slider_container<int>{"#Concurrent Frames", aCurrent, aMin, aMax, std::move(cb)});
	}

	static slider_container<int> get_number_of_presentable_images_imgui_element(int aCurrent = 3, int aMin = 2, int aMax = 8)
	{
		std::function<void(int)> cb = [](int aNumPresentatbleImages) {
			gvk::context().main_window()->set_number_of_presentable_images(aNumPresentatbleImages);
		};
		assert(aCurrent >= aMin);
		assert(aMax >= aCurrent);
		return std::move(slider_container<int>{"#Presentable Images", aCurrent, aMin, aMax, std::move(cb)});
	}

	static check_box_container get_additional_attachments_imgui_element()
	{
		std::function<void(bool)> cb = [](bool aUseDepthAtt) {
			if (aUseDepthAtt) {
				gvk::context().main_window()->set_additional_back_buffer_attachments({
					avk::attachment::declare(vk::Format::eD32Sfloat, avk::on_load::clear, avk::depth_stencil(), avk::on_store::dont_care)
				});
			}
			else {
				gvk::context().main_window()->set_additional_back_buffer_attachments({});
			}
		};

		return std::move(check_box_container{ "Use depth att.", true, std::move(cb) });
	}
};