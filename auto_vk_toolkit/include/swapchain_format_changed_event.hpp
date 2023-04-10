#pragma once

#include "event.hpp"

namespace avk
{
	/** This event occurs when the swap chain image format changes.
	 */
	class swapchain_format_changed_event : public event
	{
	public:
		swapchain_format_changed_event(window* aWindow) : mWindow{ aWindow }, mSwapChainImageFormat{ aWindow->swap_chain_image_view_at_index(0)->create_info().format } {}
		swapchain_format_changed_event(swapchain_format_changed_event&&) noexcept = default;
		swapchain_format_changed_event(const swapchain_format_changed_event&) = default;
		swapchain_format_changed_event& operator=(swapchain_format_changed_event&&) noexcept = default;
		swapchain_format_changed_event& operator=(const swapchain_format_changed_event&) = default;
		~swapchain_format_changed_event() = default;

		bool update(event_data& aData) override
		{
			if (auto currentFormat = mWindow->swap_chain_image_view_at_index(0)->create_info().format; currentFormat != mSwapChainImageFormat) {
				mSwapChainImageFormat = currentFormat;
				return true;
			}

			return false;
		}

		auto* get_window() { return mWindow; }
		const auto* get_window() const { return mWindow; }

	private:
		vk::Format mSwapChainImageFormat;
		window* mWindow;
	};

	static bool operator==(const swapchain_format_changed_event& left, const swapchain_format_changed_event& right)
	{
		return left.get_window() == right.get_window();
	}

	static bool operator!=(const swapchain_format_changed_event& left, const swapchain_format_changed_event& right)
	{
		return !(left == right);
	}
}

namespace std // Inject hash for `swapchain_format_changed_event` into std::
{
	template<> struct hash<avk::swapchain_format_changed_event>
	{
		std::size_t operator()(avk::swapchain_format_changed_event const& o) const noexcept
		{
			std::size_t h = 0;
			avk::hash_combine(h, o.get_window());
			return h;
		}
	};
}
