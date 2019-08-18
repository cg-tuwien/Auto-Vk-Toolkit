#pragma once
#include "context_vulkan_types.h"
#include "context_generic_glfw.h"
#include "window_base.h"

namespace cgb
{
	class window : public window_base
	{
		friend class generic_glfw;
		friend class opengl46;


	};
}
