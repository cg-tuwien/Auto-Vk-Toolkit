#pragma once
#include "avk/avk_error.hpp"
#include <GLFW/glfw3.h>

namespace avk
{
	/** Context-specific handle to a window.
	 *  For this context, the window is handled via GLFW.
	 */
	struct window_handle
	{
		GLFWwindow* mHandle;
	};

	/** Context-specific handle to a monitor.
	 *  For this context, the monitor is handled via GLFW.
	 */
	struct monitor_handle
	{
		/** Gets a monitor handle to the primary monitor */
		static monitor_handle primary_monitor()
		{
			return monitor_handle{ glfwGetPrimaryMonitor() };
		}

		/** Gets a monitor handle to ANY secondary monitor */
		static monitor_handle secondary_monitor()
		{
			const auto primary = primary_monitor();
			int count;
			GLFWmonitor** monitors = glfwGetMonitors(&count);
			for (int i = 0; i < count; ++i) {
				if (monitors[i] != primary.mHandle) {
					return monitor_handle{ monitors[i] };
				}
			}
			throw avk::runtime_error("No secondary monitor found");
		}

		GLFWmonitor* mHandle;
	};

	/** Different options on how to present the images in the back buffer on 
	 *	the surface. 
	 */
	enum struct presentation_mode
	{
		/** Submit images immediately to the screen */
		immediate,
		/** TBD */
		relaxed_fifo,
		/** Double buffering and wait for "vertical blank" */
		fifo,
		/** Mailbox mode */
		mailbox
	};

}
