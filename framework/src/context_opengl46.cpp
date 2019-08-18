#include "context_opengl46.h"

namespace cgb
{
	opengl46::opengl46() 
		: generic_glfw()
		, mInitializationComplete(false)
	{
	}

	bool opengl46::check_error(const char* file, int line)
	{
		bool hasError = false;
		GLenum err;
		while ((err = glGetError()) != GL_NO_ERROR)
		{
			LOG_ERROR(fmt::format("glError int[{:d}] hex[0x{:x}] in file[{}], line[{}]", err, err, file, line));
			hasError = true;
		}
		return hasError;
	}

	window* opengl46::create_window(std::string pTitle)
	{
		auto* wnd = generic_glfw::prepare_window();
		wnd->set_title(std::move(pTitle));

		wnd->mPreCreateActions.push_back([](cgb::window & w) {
			// Set several configuration parameters before actually creating the window:
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef _DEBUG
			glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

			// Set a default value for the following settings:
			glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_FALSE);
			glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
		});

		wnd->mPostCreateActions.push_back([](cgb::window& w) {
			if (!context().initialization_completed()) {
				// If context has been newly created in the current call to create_window, 
				// 1) make the newly created context current and
				// 2) use the extension loader to get the proc-addresses (which needs an active context)
				glfwMakeContextCurrent(w.handle()->mHandle);
				gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
				glfwFocusWindow(w.handle()->mHandle);
				context().mInitializationComplete = true;
			}
		});

		return wnd;
	}

	void opengl46::finish_pending_work()
	{

	}

	void opengl46::begin_composition()
	{

	}

	void opengl46::end_composition()
	{

	}

	void opengl46::begin_frame()
	{

	}

	void opengl46::end_frame()
	{

	}


}
