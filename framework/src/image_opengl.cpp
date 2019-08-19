#include <cg_base.hpp>

#include <set>

namespace cgb
{
	image_format::image_format() noexcept
	{ }

	image_format::image_format(GLenum pFormat) noexcept
		: mFormat{ pFormat }
	{ }

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

	bool has_stencil_component(const image_format& pImageFormat)
	{
		throw std::runtime_error("not implemented");
	}

	bool is_depth_format(const image_format& pImageFormat)
	{
		throw std::runtime_error("not implemented");
	}

	owning_resource<image_t> image_t::create(int pWidth, int pHeight, image_format pFormat, memory_usage pMemoryUsage, bool pUseMipMaps, int pNumLayers, context_specific_function<void(image_t&)> pAlterConfigBeforeCreation)
	{
		image_t result;
		return result;
	}

	owning_resource<image_t> image_t::create_depth(int pWidth, int pHeight, std::optional<image_format> pFormat, memory_usage pMemoryUsage, bool pUseMipMaps, int pNumLayers, context_specific_function<void(image_t&)> pAlterConfigBeforeCreation)
	{
		image_t result;
		return result;
	}

	owning_resource<image_t> image_t::create_depth_stencil(int pWidth, int pHeight, std::optional<image_format> pFormat, memory_usage pMemoryUsage, bool pUseMipMaps, int pNumLayers, context_specific_function<void(image_t&)> pAlterConfigBeforeCreation)
	{
		image_t result;
		return result;
	}
}
