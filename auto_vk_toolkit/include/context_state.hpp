#pragma once

namespace avk
{
	enum struct context_state
	{
		uninitialized				= 0x0000,
		initialization_begun		= 0x0001,
		physical_device_selected	= 0x0002,
		surfaces_created			= 0x0004,
		device_created				= 0x0008,
		fully_initialized			= 0x0020,
		frame_begun					= 0x0040,
		frame_updates_done			= 0x0080,
		frame_ended					= 0x0100,
		composition_ending			= 0x0200,
		composition_beginning		= 0x0400,
		about_to_finalize			= 0x1000,
		has_finalized				= 0x2000,
		anytime_during_or_after_init = initialization_begun | physical_device_selected | surfaces_created | device_created | fully_initialized,
		anytime_after_init_before_finalize = fully_initialized | frame_begun | frame_updates_done | frame_ended | composition_ending | composition_beginning,
		anytime_during_composition_transition = composition_ending | composition_beginning,
		anytime_during_finalization = about_to_finalize | has_finalized,
		anytime = anytime_during_or_after_init | anytime_after_init_before_finalize | anytime_during_finalization
	};

	inline context_state operator| (context_state a, context_state b)
	{
		typedef std::underlying_type<context_state>::type EnumType;
		return static_cast<context_state>(static_cast<EnumType>(a) | static_cast<EnumType>(b));
	}

	inline context_state operator& (context_state a, context_state b)
	{
		typedef std::underlying_type<context_state>::type EnumType;
		return static_cast<context_state>(static_cast<EnumType>(a) & static_cast<EnumType>(b));
	}

	inline context_state& operator |= (context_state& a, context_state b)
	{
		return a = a | b;
	}

	inline context_state& operator &= (context_state& a, context_state b)
	{
		return a = a & b;
	}
}
