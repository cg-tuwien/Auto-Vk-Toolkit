#pragma once

#include "event.hpp"

namespace avk
{
	/** This event occurs when the given window's swap chain extent does not
	 *	match the extent from the last time when update() was invoked.
	 */
	class swapchain_resized_event : public event
	{
	public:
		swapchain_resized_event(window* aWindow) : mWindow{ aWindow }, mPrevExtent{ aWindow->swap_chain_extent() } {}
		swapchain_resized_event(swapchain_resized_event&&) noexcept = default;
		swapchain_resized_event(const swapchain_resized_event&) = default;
		swapchain_resized_event& operator=(swapchain_resized_event&&) noexcept = default;
		swapchain_resized_event& operator=(const swapchain_resized_event&) = default;
		~swapchain_resized_event() = default;
		
		bool update(event_data& aData) override
		{
			auto curExtent = mWindow->swap_chain_extent();
			const auto result = mPrevExtent != curExtent;
			if (result) {
				aData.add_or_find_old_extent(mPrevExtent).mNewExtent = curExtent;
			}
			mPrevExtent = curExtent;
			return result;
		}

		auto* get_window() { return mWindow; }
		const auto* get_window() const { return mWindow; }

	private:
		window* mWindow;
		vk::Extent2D mPrevExtent;
	};

	static bool operator==(const swapchain_resized_event& left, const swapchain_resized_event& right)
	{
		return left.get_window() == right.get_window();
	}

	static bool operator!=(const swapchain_resized_event& left, const swapchain_resized_event& right)
	{
		return !(left == right);
	}
}

namespace std // Inject hash for `swapchain_resized_event` into std::
{
	template<> struct hash<avk::swapchain_resized_event>
	{
		std::size_t operator()(avk::swapchain_resized_event const& o) const noexcept
		{
			std::size_t h = 0;
			avk::hash_combine(h, o.get_window());
			return h;
		}
	};
}
