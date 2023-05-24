#pragma once

#include "event.hpp"

namespace avk
{
	/** This event occurs when the given window's swap chain image view handle
	 *	at index 0 is different from the handle at the same index from the last
	 *	time when update() was invoked.
	 */
	class swapchain_changed_event : public event
	{
	public:
		swapchain_changed_event(window* aWindow) : mWindow{ aWindow }, mPrevImageViewHandle{ aWindow->swap_chain_image_view_at_index(0)->handle() } {}
		swapchain_changed_event(swapchain_changed_event&&) noexcept = default;
		swapchain_changed_event(const swapchain_changed_event&) = default;
		swapchain_changed_event& operator=(swapchain_changed_event&&) noexcept = default;
		swapchain_changed_event& operator=(const swapchain_changed_event&) = default;
		~swapchain_changed_event() = default;
		
		bool update(event_data& aData) override
		{
			const auto curImageViewHandle = mWindow->swap_chain_image_view_at_index(0)->handle();
			const auto result = mPrevImageViewHandle != static_cast<VkImageView>(curImageViewHandle);
			if (result) {
				aData.mSwapchainImageExtent = mWindow->swap_chain_extent();
			}
			mPrevImageViewHandle = curImageViewHandle;
			return result;
		}

		auto* get_window() { return mWindow; }
		const auto* get_window() const { return mWindow; }

	private:
		window* mWindow;
		VkImageView mPrevImageViewHandle;
	};

	static bool operator==(const swapchain_changed_event& left, const swapchain_changed_event& right)
	{
		return left.get_window() == right.get_window();
	}

	static bool operator!=(const swapchain_changed_event& left, const swapchain_changed_event& right)
	{
		return !(left == right);
	}
}

namespace std // Inject hash for `swapchain_changed_event` into std::
{
	template<> struct hash<avk::swapchain_changed_event>
	{
		std::size_t operator()(avk::swapchain_changed_event const& o) const noexcept
		{
			std::size_t h = 0;
			avk::hash_combine(h, o.get_window());
			return h;
		}
	};
}
