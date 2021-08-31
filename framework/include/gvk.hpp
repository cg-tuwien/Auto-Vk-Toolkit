#pragma once
// ReSharper disable CppUnusedIncludeDirective

// ------ commonly used stuff from the standard library ------
#include <vector>
#include <deque>
#include <array>
#include <string>
#include <string_view>
#include <exception>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <stack>
#include <functional>
#include <memory>
#include <iostream>
#include <ostream>
#include <fstream>
#include <queue>
#include <algorithm>
#include <variant>
#include <iomanip>
#include <optional>
#include <typeinfo>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <cstdlib>
#include <typeindex>
#include <type_traits>
#include <utility>
#include <cstdint>
#include <chrono>
#include <filesystem>

#include <cstdio>
#include <cassert>

// ----------------------- externals -----------------------
#define FMT_HEADER_ONLY
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/chrono.h>
#include <stb_image.h>

#include <assimp/Importer.hpp>  // C++ importer interface
#include <assimp/scene.h>       // Output data structure
#include <assimp/postprocess.h> // Post processing flags
#include <assimp/anim.h>

#include <nlohmann/json.hpp>

// DEFINES:
#define GLFW_INCLUDE_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR

// The perspective projection matrix generated by GLM will use the OpenGL depth range of -1.0 to 1.0 by default.
// We need to configure it to use the Vulkan range of 0.0 to 1.0 using the GLM_FORCE_DEPTH_ZERO_TO_ONE definition. [1]
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/quaternion_common.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/ext/quaternion_exponential.hpp>
#include <glm/ext/quaternion_relational.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/mat4x4.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform2.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/compatibility.hpp>

#include <gli/load.hpp> // load DDS, KTX or KMG textures from files or memory.
#include <gli/core/flip.hpp>

// -------------------- windows include ---------------------
#ifdef _WIN32
// Include that after the assimp includes due to some conflict
// with the global scope operator :: in material.inl
#define NOMINMAX
#include <windows.h>
#endif

#define AVK_LOG_FATAL			LOG_ERROR_EM
#define AVK_LOG_ERROR			LOG_ERROR
#define AVK_LOG_WARNING			LOG_WARNING
#define AVK_LOG_INFO			LOG_INFO
#define AVK_LOG_VERBOSE			LOG_VERBOSE
#define AVK_LOG_DEBUG			LOG_DEBUG
#define AVK_LOG_DEBUG_VERBOSE	LOG_DEBUG_VERBOSE
#include "log.hpp"

#define AVK_USE_VMA
#include <avk/avk.hpp>

// -------------------- Gears-Vk includes --------------------
#include "cgb_exceptions.hpp"
#include "conversion_utils.hpp"

#include "context_state.hpp"

#include "cursor.hpp"

#include "context_generic_glfw_types.hpp"
#include "window_base.hpp"

#include "window.hpp"
#include "context_generic_glfw.hpp"

#include "math_utils.hpp"
#include "key_code.hpp"
#include "key_state.hpp"
#include "timer_frame_type.hpp"
#include "timer_interface.hpp"
#include "fixed_update_timer.hpp"
#include "varying_update_timer.hpp"
#include "input_buffer.hpp"
#include "composition_interface.hpp"

#include "vk_convenience_functions.hpp"

#include "settings.hpp"
#include "context_vulkan.hpp"

namespace gvk
{
#pragma region global data representing the currently active composition
	/**	@brief Get the current timer, which represents the current game-/render-time
	 *	\remark This is just a shortcut to @ref composition_interface::current()->time();
	 */
	inline timer_interface& time()
	{
		return composition_interface::current()->time();
	}

	/** @brief Get the current frame's input data
	 *	\remark This is just a shortcut to @ref composition_interface::current()->input();
	 */
	inline input_buffer& input()
	{
		return composition_interface::current()->input();
	}

	/** @brief Get access to the currently active objects
	 *	\remark This is just a shortcut to @ref *composition_interface::current();
	 */
	inline composition_interface* current_composition()
	{
		return composition_interface::current();
	}

	inline auto& context()
	{
		static context_vulkan sContext;
		return sContext;
	}
#pragma endregion
}

namespace gvk {
	class serializer;
}

#include "event_data.hpp"
#include "event.hpp"
#include "files_changed_event.hpp"
#include "swapchain_resized_event.hpp"
#include "swapchain_changed_event.hpp"
#include "swapchain_format_changed_event.hpp"
#include "concurrent_frames_count_changed_event.hpp"
#include "swapchain_additional_attachments_changed_event.hpp"
#include "destroying_events.hpp"
#include "updater.hpp"

#include "invokee.hpp"
#include "invoker_interface.hpp"
#include "sequential_invoker.hpp"

#include "transform.hpp"
#include "camera.hpp"
#include "quake_camera.hpp"
#include "material_config.hpp"
#include "material_gpu_data.hpp"
#include "material_gpu_data_ext.hpp"
#include "material.hpp"
#include "lightsource.hpp"
#include "lightsource_gpu_data.hpp"
#include "model_types.hpp"
#include "animation.hpp"
#include "model.hpp"
#include "orca_scene.hpp"
#include "serializer.hpp"
#include "image_data.hpp"
#include "material_image_helpers.hpp"

#include "composition.hpp"
#include "setup.hpp"

#include "imgui_manager.hpp"

#include "cp_interpolation.hpp"
#include "bezier_curve.hpp"
#include "catmull_rom_spline.hpp"
#include "quadratic_uniform_b_spline.hpp"
#include "cubic_uniform_b_spline.hpp"

// ReSharper restore CppUnusedIncludeDirective
