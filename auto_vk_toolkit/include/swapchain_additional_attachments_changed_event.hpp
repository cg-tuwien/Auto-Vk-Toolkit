#pragma once

#include "event.hpp"

namespace avk
{
	/** This event occurs when the number of additional attachments changes.
	 *
	 *  ATTENTION: Current implementation has a weak conditioning and only checks for count.
	 *  Implement -efficient- pair check between each attachment if necessary!
	 */
	class swapchain_additional_attachments_changed_event : public event
	{
	public:
		swapchain_additional_attachments_changed_event(window* aWindow) : mWindow{ aWindow }, additionalAttachmentsSize{ aWindow->get_additional_back_buffer_attachments().size() } {}
		swapchain_additional_attachments_changed_event(swapchain_additional_attachments_changed_event&&) noexcept = default;
		swapchain_additional_attachments_changed_event(const swapchain_additional_attachments_changed_event&) = default;
		swapchain_additional_attachments_changed_event& operator=(swapchain_additional_attachments_changed_event&&) noexcept = default;
		swapchain_additional_attachments_changed_event& operator=(const swapchain_additional_attachments_changed_event&) = default;
		~swapchain_additional_attachments_changed_event() = default;

		bool update(event_data& aData) override
		{
			auto currentSize = mWindow->get_additional_back_buffer_attachments().size();
			if (currentSize != additionalAttachmentsSize) {
				additionalAttachmentsSize = currentSize;
				return true;
			}

			return false;
		}

		auto* get_window() { return mWindow; }
		const auto* get_window() const { return mWindow; }

	private:
		size_t additionalAttachmentsSize;
		window* mWindow;
	};

	static bool operator==(const swapchain_additional_attachments_changed_event& left, const swapchain_additional_attachments_changed_event& right)
	{
		return left.get_window() == right.get_window();
	}

	static bool operator!=(const swapchain_additional_attachments_changed_event& left, const swapchain_additional_attachments_changed_event& right)
	{
		return !(left == right);
	}
}

namespace std // Inject hash for `swapchain_additional_attachments_changed_event` into std::
{
	template<> struct hash<avk::swapchain_additional_attachments_changed_event>
	{
		std::size_t operator()(avk::swapchain_additional_attachments_changed_event const& o) const noexcept
		{
			std::size_t h = 0;
			avk::hash_combine(h, o.get_window());
			return h;
		}
	};
}
