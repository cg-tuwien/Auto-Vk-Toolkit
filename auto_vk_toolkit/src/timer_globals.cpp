#include "timer_interface.hpp"

namespace avk
{
#pragma region global data representing the currently active composition
	/**	@brief Get the current timer, which represents the current game-/render-time
	 *	This can be nullptr! Use time() for a version which will not return nullptr.
	 */
	timer_interface*& timer_reference()
	{
		static timer_interface* sTimer = nullptr;
		return sTimer;
	}

	/**	@brief Sets a new timer.
	 *	ATTENTION: This timer must live until the end of the application!
	 */
	void set_timer(timer_interface* const aPointerToNewTimer)
	{
		timer_reference() = aPointerToNewTimer;
	}

	/**	@brief Get the current timer, which represents the current game-/render-time
	 */
	timer_interface& time()
	{
		// If there is no timer set AT THE FIRST INVOCATION, one will be set.
		// But at subsequent invocations, no further checks are performed.
		static bool sInitialCheckPerformed = []() {
			if (nullptr == timer_reference()) {
				set_default_timer();
			}
			return true;
		}();

		// In every subsequent invocation, just return whatever timer is set:
		return *timer_reference();
	}
}