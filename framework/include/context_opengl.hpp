#pragma once

// INCLUDES:
#include <glad/glad.h>
#include "context_generic_glfw.h"
#include "context_opengl46_types.h"
#include "imgui_impl_opengl3.h"

namespace cgb
{
	/**	@brief Context for OpenGL 4.6 core profile
	 *	
	 *	This context uses OpenGL 4.6 functionality and a core profile.
	 *	Some data and functionality is shared from the generic_opengl 
	 *  context, environment-related matters like window creation is
	 *	performed via GLFW, most of which is implemented in @ref generic_glfw
	 *  context.
	 */
	class opengl46 : public generic_glfw
	{
	public:
		opengl46();

		window* create_window(std::string);

		/** Will return true if the OpenGL 4.6 context has been initialized completely. */
		bool initialization_completed() const { return mInitializationComplete; }

		/**	Checks whether there is a GL-Error and logs it to the console
				 *	@return true if there was an error, false if not
				 */
		bool check_error(const char* file, int line);

		/** Used to signal the context about the beginning of a composition */
		void begin_composition();

		/** Used to signal the context about the end of a composition */
		void end_composition();

		/** Used to signal the context about the beginning of a new frame */
		void begin_frame();

		/** Used to signal the context about the end of a frame */
		void end_frame();

		/** Completes all pending work on the device, blocks the current thread until then. */
		void finish_pending_work();

	private:
		bool mInitializationComplete;
	};
}
