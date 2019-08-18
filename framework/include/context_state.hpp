#pragma once

namespace cgb
{
	enum struct context_state
	{
		halfway_initialized			= 0x0001,
		fully_initialized			= 0x0002,
		frame_begun					= 0x0010,
		frame_updates_done			= 0x0020,
		frame_ended					= 0x0040,
		composition_ending			= 0x0100,
		composition_beginning		= 0x0200,
		about_to_finalize			= 0x1000,
		has_finalized				= 0x2000,
		anytime_during_or_after_init = halfway_initialized | fully_initialized,
		anytime_after_init_before_finalize = fully_initialized | frame_begun | frame_updates_done | frame_ended | composition_ending | composition_beginning,
		anytime_during_composition_transition = composition_ending | composition_beginning,
		anytime_during_finalization = about_to_finalize | has_finalized,
		anytime = halfway_initialized | fully_initialized | frame_begun | frame_updates_done | frame_ended | composition_ending | composition_beginning | about_to_finalize | has_finalized
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
