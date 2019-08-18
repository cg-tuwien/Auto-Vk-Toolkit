#pragma once

namespace cgb
{
	/**	The update type for timers
	*/
	enum struct timer_frame_type
	{
		none	= 0x00,
		/** Fixed timestep, "simulation timestep" */
		fixed	= 0x01,
		/** varying timestep, "render timestep" */
		varying = 0x02,
		/** any sort of timestep */
		any		= fixed | varying
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
