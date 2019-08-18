#include "context_opengl46_types.h"
#include <set>

namespace cgb
{
	window::window() 
		: window_base()
		, mPreCreateActions()
	{}

	window::~window() 
	{
		if (mHandle) {
			context().close_window(*this);
			mHandle = std::nullopt;
		}
	}

	window::window(window&& other) noexcept 
		: window_base(std::move(other))
		, mPreCreateActions(std::move(other.mPreCreateActions))
	{
	}

	window& window::operator= (window&& other) noexcept
	{
		window_base::operator=(std::move(other));
		mPreCreateActions = std::move(other.mPreCreateActions);
		return *this;
	}

	void window::request_srgb_framebuffer(bool pRequestSrgb)
	{
		switch (pRequestSrgb) {
		case true:
			mPreCreateActions.push_back(
				[](cgb::window & w) {
				glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
			});
			break;
		default:
			mPreCreateActions.push_back(
				[](cgb::window & w) {
				glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_FALSE);
			});
			break;
		}
		// If the window has already been created, the new setting can't 
		// be applied unless the window is being recreated.
		if (is_alive()) {
			mRecreationRequired = true;
		}
	}

	void set_number_of_presentable_images(uint32_t pNumImages)
	{
		// NOP
		LOG_WARNING("set_number_of_presentable_images is not supported by the OpenGL context.");
	}

	void window::set_presentaton_mode(cgb::presentation_mode pMode)
	{
		switch (pMode) {
		case cgb::presentation_mode::immediate:
			mPreCreateActions.push_back(
				[](cgb::window & w) {
				glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_FALSE);
			});
			mPostCreateActions.push_back(
				[](cgb::window & w) {
				glfwSwapInterval(0);
			});
			break;
		case cgb::presentation_mode::vsync:
			mPreCreateActions.push_back(
				[](cgb::window & w) {
				glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
			});
			mPostCreateActions.push_back(
				[](cgb::window & w) {
				glfwSwapInterval(1);
			});
			break;
		default:
			mPreCreateActions.push_back(
				[](cgb::window & w) {
				glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
			});
			mPostCreateActions.push_back(
				[](cgb::window & w) {
				glfwSwapInterval(0);
			});
			break;
		}
		// If the window has already been created, the new setting can't 
		// be applied unless the window is being recreated.
		if (is_alive()) {
			mRecreationRequired = true;
		}
	}

	void window::set_number_of_samples(int pNumSamples)
	{
		mPreCreateActions.push_back(
			[samples = pNumSamples](cgb::window & w) {
			glfwWindowHint(GLFW_SAMPLES, samples);
		});
		// If the window has already been created, the new setting can't 
		// be applied unless the window is being recreated.
		if (is_alive()) {
			mRecreationRequired = true;
		}
	}

	void window::open()
	{
		for (const auto& fu : mPreCreateActions) {
			fu(*this);
		}

		// Share the graphics context between all windows
		auto* sharedContex = context().get_window_for_shared_context();
		// Bring window into existance:
		auto* handle = glfwCreateWindow(mRequestedSize.mWidth, mRequestedSize.mHeight,
						 mTitle.c_str(),
						 mMonitor.has_value() ? mMonitor->mHandle : nullptr,
						 sharedContex);
		if (nullptr == handle) {
			// No point in continuing
			throw new std::runtime_error("Failed to create window with the title '" + mTitle + "'");
		}
		mHandle = window_handle{ handle };

		// Finish initialization of the window:
		for (const auto& action : mPostCreateActions) {
			action(*this);
		}
	}

	bool is_srgb_format(const image_format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<GLint> srgbFormats = {
			GL_SRGB,
			GL_SRGB8,
			GL_COMPRESSED_SRGB,
			GL_SRGB_ALPHA,
			GL_SRGB8_ALPHA8,
			GL_COMPRESSED_SRGB_ALPHA
		};
		auto it = std::find(std::begin(srgbFormats), std::end(srgbFormats), pImageFormat.mFormat);
		return it != srgbFormats.end();
	}

	bool is_uint8_format(const image_format& pImageFormat)
	{
		// Note: Currently, the compressed formats are ignored => could/should be added in the future, maybe
		// TODO: sRGB-formats are assumed to be uint8-formats (not signed int8-formats) => is that true?
		static std::set<GLint> uint8Formats = {
			GL_R8,
			GL_RG8,
			GL_RGB,
			GL_RGB8,
			GL_RGBA,
			GL_RGBA8,
			GL_BGR,
			GL_BGRA,
			GL_SRGB,
			GL_SRGB8,
			GL_COMPRESSED_SRGB,
			GL_SRGB_ALPHA,
			GL_SRGB8_ALPHA8,
			GL_COMPRESSED_SRGB_ALPHA
		};
		auto it = std::find(std::begin(uint8Formats), std::end(uint8Formats), pImageFormat.mFormat);
		return it != uint8Formats.end();
	}

	bool is_int8_format(const image_format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<GLint> int8Formats = {
			// TODO: signed formats
		};
		auto it = std::find(std::begin(int8Formats), std::end(int8Formats), pImageFormat.mFormat);
		return it != int8Formats.end();
	}

	bool is_uint16_format(const image_format& pImageFormat)
	{
		// Note: Currently, the compressed formats are ignored => could/should be added in the future, maybe
		static std::set<GLint> uint16Formats = {
			GL_R16,
			GL_RG16,
			GL_RGB16,
			GL_RGBA16
		};
		auto it = std::find(std::begin(uint16Formats), std::end(uint16Formats), pImageFormat.mFormat);
		return it != uint16Formats.end();
	}

	bool is_int16_format(const image_format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<GLint> int16Formats = {
			// TODO: signed formats
		};
		auto it = std::find(std::begin(int16Formats), std::end(int16Formats), pImageFormat.mFormat);
		return it != int16Formats.end();
	}

	bool is_uint32_format(const image_format& pImageFormat)
	{
		// Note: Currently, the compressed formats are ignored => could/should be added in the future, maybe
		static std::set<GLint> uint32Format = {
			// TODO: ?
		};
		auto it = std::find(std::begin(uint32Format), std::end(uint32Format), pImageFormat.mFormat);
		return it != uint32Format.end();
	}

	bool is_int32_format(const image_format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<GLint> int32Format = {
			// TODO: ?
		};
		auto it = std::find(std::begin(int32Format), std::end(int32Format), pImageFormat.mFormat);
		return it != int32Format.end();
	}

	bool is_float16_format(const image_format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<GLint> float16Formats = {
			GL_R16F,
			GL_RG16F,
			GL_RGB16F,
			GL_RGBA16F
		};
		auto it = std::find(std::begin(float16Formats), std::end(float16Formats), pImageFormat.mFormat);
		return it != float16Formats.end();
	}

	bool is_float32_format(const image_format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<GLint> float32Formats = {
			GL_R32F,
			GL_RG32F,
			GL_RGB32F,
			GL_RGBA32F
		};
		auto it = std::find(std::begin(float32Formats), std::end(float32Formats), pImageFormat.mFormat);
		return it != float32Formats.end();
	}

	bool is_float64_format(const image_format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<GLint> float64Formats = {
			// TODO: ?
		};
		auto it = std::find(std::begin(float64Formats), std::end(float64Formats), pImageFormat.mFormat);
		return it != float64Formats.end();
	}

	bool is_rgb_format(const image_format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<GLint> rgbFormats = {
			GL_RGB,
			GL_RGB8,
			GL_RGB16,
			GL_RGB16F,
			GL_RGB32F,
			GL_SRGB,
			GL_SRGB8,
			GL_COMPRESSED_SRGB
		};
		auto it = std::find(std::begin(rgbFormats), std::end(rgbFormats), pImageFormat.mFormat);
		return it != rgbFormats.end();
	}

	bool is_rgba_format(const image_format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<GLint> rgbaFormats = {
			GL_RGBA,
			GL_RGBA8,
			GL_RGBA16,
			GL_RGBA16F,
			GL_RGBA32F,
			GL_SRGB_ALPHA,
			GL_SRGB8_ALPHA8,
			GL_COMPRESSED_SRGB_ALPHA
		};
		auto it = std::find(std::begin(rgbaFormats), std::end(rgbaFormats), pImageFormat.mFormat);
		return it != rgbaFormats.end();
	}

	bool is_argb_format(const image_format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<GLint> argbFormats = {
			// TODO: ?
		};
		auto it = std::find(std::begin(argbFormats), std::end(argbFormats), pImageFormat.mFormat);
		return it != argbFormats.end();
	}

	bool is_bgr_format(const image_format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<GLint> bgrFormats = {
			GL_BGR
		};
		auto it = std::find(std::begin(bgrFormats), std::end(bgrFormats), pImageFormat.mFormat);
		return it != bgrFormats.end();
	}

	bool is_bgra_format(const image_format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<GLint> bgraFormats = {
			GL_BGRA
		};
		auto it = std::find(std::begin(bgraFormats), std::end(bgraFormats), pImageFormat.mFormat);
		return it != bgraFormats.end();
	}

	bool is_abgr_format(const image_format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<GLint> abgrFormats = {
			GL_ABGR_EXT
		};
		auto it = std::find(std::begin(abgrFormats), std::end(abgrFormats), pImageFormat.mFormat);
		return it != abgrFormats.end();
	}
}
