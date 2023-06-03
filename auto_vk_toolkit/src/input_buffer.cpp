
#include "input_buffer.hpp"

namespace avk
{
	extern bool are_we_on_the_main_thread();

	void input_buffer::reset(std::optional<window> pWindow) // TODO: Where is this used??
	{
		std::fill(std::begin(mKeyboardKeys), std::end(mKeyboardKeys), key_state::none);
		std::fill(std::begin(mMouseKeys), std::end(mMouseKeys), key_state::none);
		mWindow = nullptr;
		mCursorPosition = { 0.0, 0.0 };
		mDeltaCursorPosition = { 0.0, 0.0 };
		mScrollDelta = { 0.0, 0.0 };
		mCursorDisabled = false;
		mCenterCursorPosition = std::nullopt;
		mSetCursorPosition = std::nullopt;
		mSetCursorMode = std::nullopt;
		mCharacters.clear();
	}

	void input_buffer::prepare_for_next_frame(input_buffer& pFrontBufferToBe, input_buffer& pBackBufferToBe, window* pWindow)
	{
		pWindow->update_cursor_position();
		// pFrontBufferToBe = previous back buffer
		// pBackBufferToBe = previous front buffer

		// Handle all the keyboard input
		for (size_t i = 0; i < pFrontBufferToBe.mKeyboardKeys.size(); ++i) {
			// Retain those down-states:
			pBackBufferToBe.mKeyboardKeys[i] = (pFrontBufferToBe.mKeyboardKeys[i] & key_state::down);
		}
		// Handle all the mouse button input
		for (size_t i = 0; i < pFrontBufferToBe.mMouseKeys.size(); ++i) {
			// Retain those down-states:
			pBackBufferToBe.mMouseKeys[i] = (pFrontBufferToBe.mMouseKeys[i] & key_state::down);
		}

		// Handle window changes (different window in focus) and other window-related actions
		pFrontBufferToBe.mWindow = pWindow;
		pFrontBufferToBe.mCursorDisabled = pFrontBufferToBe.mWindow->is_cursor_disabled();
		pFrontBufferToBe.mCursorPosition = pFrontBufferToBe.mWindow->cursor_position();
		if (pFrontBufferToBe.mWindow == pBackBufferToBe.mWindow && pBackBufferToBe.is_cursor_disabled() == pFrontBufferToBe.is_cursor_disabled()) {
			pFrontBufferToBe.mDeltaCursorPosition = pBackBufferToBe.mCursorPosition - pFrontBufferToBe.mCursorPosition;
		}
		else { // Window has changed!
			pFrontBufferToBe.mDeltaCursorPosition = { 0.0, 0.0 };
		}

		if (pBackBufferToBe.mCenterCursorPosition.has_value() || pBackBufferToBe.mSetCursorPosition.has_value()) {
			assert(are_we_on_the_main_thread());
			if (pBackBufferToBe.mCenterCursorPosition.has_value()) {
				auto res = pWindow->resolution();
				pWindow->set_cursor_pos({ res.x / 2.0, res.y / 2.0 });
			}
			else {
				pWindow->set_cursor_pos({ pBackBufferToBe.mSetCursorPosition->x, pBackBufferToBe.mSetCursorPosition->y });
			}
			// Optimistic approach: Do not query GLFW for actual cursor position afterwards
			// BUT (important!), set both buffers to the center coordinates (because of the delta position)!
			pFrontBufferToBe.mCursorPosition = pBackBufferToBe.mCursorPosition = pWindow->cursor_position();
			// Mark action as done:
			pBackBufferToBe.mCenterCursorPosition = std::nullopt;
			pBackBufferToBe.mSetCursorPosition = std::nullopt;
		}

		if (pBackBufferToBe.mSetCursorMode.has_value()) {
			assert(are_we_on_the_main_thread());
			pWindow->set_cursor_mode(pBackBufferToBe.mSetCursorMode.value());
			// Mark action as done:
			pBackBufferToBe.mSetCursorMode = std::nullopt;
		}

		// Scroll delta is always a relative amount and filled into the back-buffer by the GLFW context,
		//  i.e. no need to alter it here, just reset it for the back-buffer.
		pBackBufferToBe.mScrollDelta = { 0.0, 0.0 };
		pBackBufferToBe.mCharacters.clear();
	}

	bool input_buffer::key_pressed(key_code pKey)
	{
		return (mKeyboardKeys[static_cast<size_t>(pKey)] & key_state::pressed) != key_state::none;
	}

	bool input_buffer::key_released(key_code pKey)
	{
		return (mKeyboardKeys[static_cast<size_t>(pKey)] & key_state::released) != key_state::none;
	}

	bool input_buffer::key_down(key_code pKey)
	{
		return (mKeyboardKeys[static_cast<size_t>(pKey)] & key_state::down) != key_state::none;
	}

	bool input_buffer::mouse_button_pressed(uint8_t pButtonIndex)
	{
		return (mMouseKeys[static_cast<size_t>(pButtonIndex)] & key_state::pressed) != key_state::none;
	}

	bool input_buffer::mouse_button_released(uint8_t pButtonIndex)
	{
		return (mMouseKeys[static_cast<size_t>(pButtonIndex)] & key_state::released) != key_state::none;
	}

	bool input_buffer::mouse_button_down(uint8_t pButtonIndex)
	{
		return (mMouseKeys[static_cast<size_t>(pButtonIndex)] & key_state::down) != key_state::none;
	}

	const glm::dvec2& input_buffer::cursor_position() const
	{
		return mCursorPosition;
	}

	const glm::dvec2& input_buffer::delta_cursor_position() const
	{
		return mDeltaCursorPosition;
	}

	const glm::dvec2& input_buffer::scroll_delta() const
	{
		return mScrollDelta;
	}

	void input_buffer::set_cursor_mode(cursor aCursorModeToBeSet)
	{
		mSetCursorMode = aCursorModeToBeSet;
	}

	bool input_buffer::is_cursor_disabled() const
	{
		return mCursorDisabled;
	}

	void input_buffer::center_cursor_position()
	{
		mCenterCursorPosition = true;
	}

	void input_buffer::set_cursor_position(glm::dvec2 pNewPosition)
	{
		mSetCursorPosition = pNewPosition;
	}

	const std::vector<unsigned int>& input_buffer::entered_characters() const
	{
		return mCharacters;
	}
}