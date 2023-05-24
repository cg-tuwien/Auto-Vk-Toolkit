#pragma once
#include "key_code.hpp"
#include "key_state.hpp"

namespace avk
{
	/** @brief Contains all the input data of a frame
	 *
	 *	This structure will be filled during a frame, so that it contains
	 *	the input of the current/last/whichever frame.
	 */
	class input_buffer
	{
		friend class context_generic_glfw;
		friend class context_vulkan;

	public:
		/** Resets all the input values to a state representing no input.
		 *	If a window is passed, the cursor is set to the cursor position
		 *	w.r.t. to that window.
		 *	(This could be useful at the beginning of a frame)
		 */
		void reset(std::optional<window> pWindow = std::nullopt);

		/** @brief Keyboard key pressed-down?
		 *
		 *	True if the given keyboard key has been pressed down in the 
		 *	current input-frame 
		 */
		bool key_pressed(key_code);

		/** @brief Keyboard key released?
		 *
		 *	True if the given keyboard key has been released in the
		 *	current input-frame
		 */
		bool key_released(key_code);

		/** @brief Keyboard key held down?
		 *
		 *	True if the given keyboard key is (possibly repeatedly) 
		 *	held down in the current input-frame.
		 *	
		 *	\remark If @ref key_pressed is true, key_down will 
		 *	be true either in any case for the given @ref key_code.
		 */
		bool key_down(key_code);

		/** @brief Mouse button pressed-down?
		 *
		 *	True if the mouse button with the given index has been 
		 *	pressed down in the current input-frame
		 */
		bool mouse_button_pressed(uint8_t);

		/** @brief Mouse button released?
		 *
		 *	True if the mouse button with the given index has been
		 *	released in the current input-frame
		 */
		bool mouse_button_released(uint8_t);

		/** @brief Mouse button held down?
		 *
		 *	True if the mouse button with the given index is (possibly 
		 *	repeatedly) held down in the current input-frame
		 */
		bool mouse_button_down(uint8_t);

		/** @brief Cursor position w.r.t. the window which is currently in focus
		 *	Returns the position of the cursor w.r.t. the given window.
		 */
		const glm::dvec2& cursor_position() const;

		/** @brief The amount of how much the cursor position has changed w.r.t. 
		 *	to the previous frame.
		 */
		const glm::dvec2& delta_cursor_position() const;

		/** @brief Mouse wheel's scroll delta
		 *	Returns the accumulated scrolling delta performed with
		 *	the mouse wheel during the current input frame.
		 */
		const glm::dvec2& scroll_delta() const;

		/** Sets the cursor mode */
		void set_cursor_mode(cursor aCursorModeToBeSet);

		/** Returns if the cursor is hidden or not */
		bool is_cursor_disabled() const;

		/** Positions the cursor in the center of the screen */
		void center_cursor_position();

		/** Positions the cursor at the new coordinates */
		void set_cursor_position(glm::dvec2 pNewPosition);

		/** Prepares this input buffer for the next frame based on data of
		 *	the previous frame. This means that key-down states are preserved.
		 */
		static void prepare_for_next_frame(input_buffer& pFrontBufferToBe, input_buffer& pBackBufferToBe, window* pWindow = nullptr);

		/** Vector of characters that have been entered during the last frame
		 */
		const std::vector<unsigned int>& entered_characters() const;

	private:
		/** Keyboard button states */
		std::array<key_state, static_cast<size_t>(key_code::max_value)> mKeyboardKeys;

		/** Mouse button states */
		std::array<key_state, 8> mMouseKeys;

		/** The window which is in focus when this buffer is active. */
		window* mWindow;

		/** Position of the mouse cursor */
		glm::dvec2 mCursorPosition;

		/** How much the mouse cursor has moved w.r.t. the previous frame */
		glm::dvec2 mDeltaCursorPosition;

		/** Scrolling wheel position data */
		glm::dvec2 mScrollDelta;

		/** True if the cursor is disabled, false otherwise */
		bool mCursorDisabled;

		/** Has value if the cursor's position should be centered */
		std::optional<bool> mCenterCursorPosition;

		/** Has value if the cursor's position should be changed */
		std::optional<glm::dvec2> mSetCursorPosition;

		/** Has value if the cursor's mode should be changed */
		std::optional<cursor> mSetCursorMode;

		/** Contains characters that have been entered during the last frame */
		std::vector<unsigned int> mCharacters;
	};
}
