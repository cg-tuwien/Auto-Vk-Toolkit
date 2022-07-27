#pragma once
#include <auto_vk_toolkit.hpp>

namespace avk // Inject into avk::
{
	// f32
	template <>	inline vk::Format format_for<glm::vec<4, glm::f32, glm::defaultp>>() { return vk::Format::eR32G32B32A32Sfloat; }
	template <> inline vk::Format format_for<glm::vec<3, glm::f32, glm::defaultp>>() { return vk::Format::eR32G32B32Sfloat; }
	template <>	inline vk::Format format_for<glm::vec<2, glm::f32, glm::defaultp>>() { return vk::Format::eR32G32Sfloat; }
	template <>	inline vk::Format format_for<glm::vec<1, glm::f32, glm::defaultp>>() { return vk::Format::eR32Sfloat; }
	// f64
	template <>	inline vk::Format format_for<glm::vec<4, glm::f64, glm::defaultp>>() { return vk::Format::eR64G64B64A64Sfloat; }
	template <>	inline vk::Format format_for<glm::vec<3, glm::f64, glm::defaultp>>() { return vk::Format::eR64G64B64Sfloat; }
	template <>	inline vk::Format format_for<glm::vec<2, glm::f64, glm::defaultp>>() { return vk::Format::eR64G64Sfloat; }
	template <>	inline vk::Format format_for<glm::vec<1, glm::f64, glm::defaultp>>() { return vk::Format::eR64Sfloat; }
	// i8
	template <>	inline vk::Format format_for<glm::vec<4, glm::i8, glm::defaultp>>() { return vk::Format::eR8G8B8A8Sint; }
	template <>	inline vk::Format format_for<glm::vec<3, glm::i8, glm::defaultp>>() { return vk::Format::eR8G8B8Sint; }
	template <>	inline vk::Format format_for<glm::vec<2, glm::i8, glm::defaultp>>() { return vk::Format::eR8G8Sint; }
	template <>	inline vk::Format format_for<glm::vec<1, glm::i8, glm::defaultp>>() { return vk::Format::eR8Sint; }
	// i16
	template <>	inline vk::Format format_for<glm::vec<4, glm::i16, glm::defaultp>>() { return vk::Format::eR16G16B16A16Sint; }
	template <>	inline vk::Format format_for<glm::vec<3, glm::i16, glm::defaultp>>() { return vk::Format::eR16G16B16Sint; }
	template <>	inline vk::Format format_for<glm::vec<2, glm::i16, glm::defaultp>>() { return vk::Format::eR16G16Sint; }
	template <>	inline vk::Format format_for<glm::vec<1, glm::i16, glm::defaultp>>() { return vk::Format::eR16Sint; }
	// i32
	template <>	inline vk::Format format_for<glm::vec<4, glm::i32, glm::defaultp>>() { return vk::Format::eR32G32B32A32Sint; }
	template <>	inline vk::Format format_for<glm::vec<3, glm::i32, glm::defaultp>>() { return vk::Format::eR32G32B32Sint; }
	template <>	inline vk::Format format_for<glm::vec<2, glm::i32, glm::defaultp>>() { return vk::Format::eR32G32Sint; }
	template <>	inline vk::Format format_for<glm::vec<1, glm::i32, glm::defaultp>>() { return vk::Format::eR32Sint; }
	// i64
	template <>	inline vk::Format format_for<glm::vec<4, glm::i64, glm::defaultp>>() { return vk::Format::eR64G64B64A64Sint; }
	template <>	inline vk::Format format_for<glm::vec<3, glm::i64, glm::defaultp>>() { return vk::Format::eR64G64B64Sint; }
	template <>	inline vk::Format format_for<glm::vec<2, glm::i64, glm::defaultp>>() { return vk::Format::eR64G64Sint; }
	template <>	inline vk::Format format_for<glm::vec<1, glm::i64, glm::defaultp>>() { return vk::Format::eR64Sint; }
	// u8
	template <> inline vk::Format format_for<glm::vec<4, glm::u8, glm::defaultp>>() { return vk::Format::eR8G8B8A8Uint; }
	template <> inline vk::Format format_for<glm::vec<3, glm::u8, glm::defaultp>>() { return vk::Format::eR8G8B8Uint; }
	template <> inline vk::Format format_for<glm::vec<2, glm::u8, glm::defaultp>>() { return vk::Format::eR8G8Uint; }
	template <> inline vk::Format format_for<glm::vec<1, glm::u8, glm::defaultp>>() { return vk::Format::eR8Uint; }
	// u16
	template <>	inline vk::Format format_for<glm::vec<4, glm::u16, glm::defaultp>>() { return vk::Format::eR16G16B16A16Uint; }
	template <>	inline vk::Format format_for<glm::vec<3, glm::u16, glm::defaultp>>() { return vk::Format::eR16G16B16Uint; }
	template <>	inline vk::Format format_for<glm::vec<2, glm::u16, glm::defaultp>>() { return vk::Format::eR16G16Uint; }
	template <>	inline vk::Format format_for<glm::vec<1, glm::u16, glm::defaultp>>() { return vk::Format::eR16Uint; }
	// u32
	template <>	inline vk::Format format_for<glm::vec<4, glm::u32, glm::defaultp>>() { return vk::Format::eR32G32B32A32Uint; }
	template <>	inline vk::Format format_for<glm::vec<3, glm::u32, glm::defaultp>>() { return vk::Format::eR32G32B32Uint; }
	template <>	inline vk::Format format_for<glm::vec<2, glm::u32, glm::defaultp>>() { return vk::Format::eR32G32Uint; }
	template <>	inline vk::Format format_for<glm::vec<1, glm::u32, glm::defaultp>>() { return vk::Format::eR32Uint; }
	// u64
	template <>	inline vk::Format format_for<glm::vec<4, glm::u64, glm::defaultp>>() { return vk::Format::eR64G64B64A64Uint; }
	template <>	inline vk::Format format_for<glm::vec<3, glm::u64, glm::defaultp>>() { return vk::Format::eR64G64B64Uint; }
	template <>	inline vk::Format format_for<glm::vec<2, glm::u64, glm::defaultp>>() { return vk::Format::eR64G64Uint; }
	template <>	inline vk::Format format_for<glm::vec<1, glm::u64, glm::defaultp>>() { return vk::Format::eR64Uint; }
}
