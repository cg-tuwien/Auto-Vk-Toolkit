#pragma once

namespace avk
{
	/** Each entry represents one key 
	 */
	enum struct key_code
	{
		none			= 0,
		unknown,
		space,
		apostrophe,
		comma,
		minus,
		period,
		slash,
		num0,
		num1,
		num2,
		num3,
		num4,
		num5,
		num6,
		num7,
		num8,
		num9,
		semicolon,
		equal,
		a,
		b,
		c,
		d,
		e,
		f,
		g,
		h,
		i,
		j,
		k,
		l,
		m,
		n,
		o,
		p,
		q,
		r,
		s,
		t,
		u,
		v,
		w,
		x,
		y,
		z,
		left_bracket,
		backslash,
		right_bracket,
		grave_accent,
		world_1,
		world_2,
		escape,
		enter,
		tab,
		backspace,
		insert,
		del,
		right,
		left,
		down,
		up,
		page_up,
		page_down,
		home,
		end,
		caps_lock,
		scroll_lock,
		num_lock,
		print_screen,
		pause,
		f1,
		f2,
		f3,
		f4,
		f5,
		f6,
		f7,
		f8,
		f9,
		f10,
		f11,
		f12,
		f13,
		f14,
		f15,
		f16,
		f17,
		f18,
		f19,
		f20,
		f21,
		f22,
		f23,
		f24,
		f25,
		numpad_0,
		numpad_1,
		numpad_2,
		numpad_3,
		numpad_4,
		numpad_5,
		numpad_6,
		numpad_7,
		numpad_8,
		numpad_9,
		numpad_decimal,
		numpad_divide,
		numpad_multiply,
		numpad_subtract,
		numpad_add,
		numpad_enter,
		numpad_equal,
		left_shift,
		left_control,
		left_alt,
		left_super,
		right_shift,
		right_control,
		right_alt,
		right_super,
		menu,
		max_value
	};

	inline key_code operator| (key_code a, key_code b)
	{
		typedef std::underlying_type<key_code>::type EnumType;
		return static_cast<key_code>(static_cast<EnumType>(a) | static_cast<EnumType>(b));
	}

	inline key_code operator& (key_code a, key_code b)
	{
		typedef std::underlying_type<key_code>::type EnumType;
		return static_cast<key_code>(static_cast<EnumType>(a) & static_cast<EnumType>(b));
	}

	inline key_code& operator |= (key_code& a, key_code b)
	{
		return a = a | b;
	}

	inline key_code& operator &= (key_code& a, key_code b)
	{
		return a = a & b;
	}

	inline auto index_of(key_code c)
	{
		typedef std::underlying_type<key_code>::type EnumType;
		return static_cast<EnumType>(c);
	}

}
