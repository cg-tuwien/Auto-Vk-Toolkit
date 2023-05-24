#pragma once

#include <type_traits>

namespace avk
{
	/**	The update type for timers
	*/
	enum struct timer_frame_type
	{
		none	= 0x00,
		/** Update timestep, "simulation timestep" */
		update	= 0x01,
		/** Render timestep, "render timestep" */
		render	= 0x02,
		/** any sort of timestep */
		any		= update | render
	};

	inline timer_frame_type operator| (timer_frame_type a, timer_frame_type b)
	{
		typedef std::underlying_type<timer_frame_type>::type EnumType;
		return static_cast<timer_frame_type>(static_cast<EnumType>(a) | static_cast<EnumType>(b));
	}

	inline timer_frame_type operator& (timer_frame_type a, timer_frame_type b)
	{
		typedef std::underlying_type<timer_frame_type>::type EnumType;
		return static_cast<timer_frame_type>(static_cast<EnumType>(a) & static_cast<EnumType>(b));
	}

	inline timer_frame_type& operator |= (timer_frame_type& a, timer_frame_type b)
	{
		return a = a | b;
	}

	inline timer_frame_type& operator &= (timer_frame_type& a, timer_frame_type b)
	{
		return a = a & b;
	}
}
