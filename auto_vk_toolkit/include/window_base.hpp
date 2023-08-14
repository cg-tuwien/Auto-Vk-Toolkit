#pragma once
#include "auto_vk_toolkit.hpp"
#include "cursor.hpp"
#include "context_generic_glfw_types.hpp"

namespace avk
{
	/** Dimensions of a window */
	struct window_size
	{
		window_size(int pWidth, int pHeight)
			: mWidth{ pWidth }
			, mHeight{ pHeight }
		{}

		int mWidth;
		int mHeight;
	};

	class window_base
	{
		friend class context_generic_glfw;
	public:
		window_base();
		window_base(window_base&&) noexcept;
		window_base(const window_base&) = delete;
		window_base& operator =(window_base&&) noexcept;
		window_base& operator =(const window_base&) = delete;
		~window_base();

		/** Returns whether or not this window is currently in use and hence, may not be closed. */
		bool is_in_use() const { return mIsInUse; }

		/** @brief Consecutive ID, identifying a window.
		 *	First window will get the ID=0, second one ID=1, etc.  */
		uint32_t id() const { return mWindowId; }

		/** Returns the window handle or std::nullopt if
		 *	it wasn't constructed successfully, has been moved from,
		 *	or has been destroyed. */
		std::optional<window_handle> handle() const { return mHandle; }

		/** Perform whatever initialization is necessary after the window has been opened. */
		void initialize_after_open();

		/** Returns the aspect ratio of the window, which is width/height */
		float aspect_ratio() const;

		/** The window title */
		const std::string& title() const { return mTitle; }

		/** Returns the monitor handle or std::nullopt if there is
		 *  no monitor assigned to this window (e.g. not running
		 *  in full-screen mode.
		 */
		std::optional<monitor_handle> monitor() const { return mMonitor; }

		/**	Returns true if the input of this window will be regarded,
		 *	false if the input of this window will be ignored.
		 */
		bool is_input_enabled() const { return mIsInputEnabled; }

		/** Sets whether or not the window is in use. Setting this to true has the
		 *	effect that the window can not be closed for the time being.
		 */
		void set_is_in_use(bool value);

		/** Set a new resolution for this window. This will also update
		 *  this window's underlying framebuffer
		 *  TODO: Resize underlying framebuffer!
		 */
		void set_resolution(window_size pExtent);

		/**	Updates the resolution based on the current window size.
		 */
		void update_resolution();

		/** Set a new title */
		void set_title(std::string pTitle);

		/** Enable or disable input handling of this window */
		void set_is_input_enabled(bool pValue);

		/** Indicates whether or not this window has already been created. */
		bool is_alive() const { return mHandle.has_value(); }

		/** Sets this window to fullscreen mode
		 *	@param	pOnWhichMonitor	Handle to the monitor where to present the window in full screen mode
		 */
		void switch_to_fullscreen_mode(monitor_handle pOnWhichMonitor = monitor_handle::primary_monitor());

		/** Switches to windowed mode by removing this window's monitor assignment */
		void switch_to_windowed_mode();

		/** Sets a specific cursor mode */
		void set_cursor_mode(cursor aCursorMode);

		/** Sets the cursor to the given coordinates */
		void set_cursor_pos(glm::dvec2 pCursorPos);

		void update_cursor_position();

		/** Get the cursor position w.r.t. the given window
		 */
		glm::dvec2 cursor_position() const;

		/** Determine the window's extent
		 */
		glm::uvec2 resolution() const;

		/** Returns whether or not the cursor is hidden
		 *	Threading: This method must always be called from the main thread
		 */
		bool is_cursor_disabled() const;

		/** Returns true if the window should be closed.
		 *  Attention: Could be that a positive result is returned with a delay of 1 frame.
		 */
		bool should_be_closed();

	protected:
		/** Static variable which holds the ID that the next window will get assigned */
		static uint32_t mNextWindowId;

		/** A flag indicating if this window is currently in use and hence, may not be closed */
		bool mIsInUse;

		/** Unique window id */
		uint32_t mWindowId;

		/** Handle of this window */
		std::optional<window_handle> mHandle;

		/** This window's title */
		std::string mTitle;

		/** Monitor this window is attached to, if set */
		std::optional<monitor_handle> mMonitor;

		/** A flag which tells if this window is enabled for receiving input (w.r.t. a running composition) */
		bool mIsInputEnabled;

		// The requested window size which only has effect BEFORE the window was created
		window_size mRequestedSize;

		// The position of the cursor
		glm::dvec2 mCursorPosition;

		// The scroll position
		glm::dvec2 mScrollPosition;

		// The window's resolution
		glm::uvec2 mResolution;

		// Whether or not the cursor is operating in disabled-mode
		cursor mCursorMode;

		// Initially false, set to true if window should close.
		bool mShouldClose;
	};
}