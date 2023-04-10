#pragma once


namespace avk
{
	/** @brief Represents the state(s) of a button
	 */
	enum struct key_state
	{
		none        = 0x00,
		pressed     = 0x01,
		down        = 0x02,
		released    = 0x04
	};

	inline key_state operator| (key_state a, key_state b)
	{
		typedef std::underlying_type<key_state>::type EnumType;
		return static_cast<key_state>(static_cast<EnumType>(a) | static_cast<EnumType>(b));
	}

	inline key_state operator& (key_state a, key_state b)
	{
		typedef std::underlying_type<key_state>::type EnumType;
		return static_cast<key_state>(static_cast<EnumType>(a) & static_cast<EnumType>(b));
	}

	inline key_state operator| (key_state a, std::underlying_type<key_state>::type b)
	{
		typedef std::underlying_type<key_state>::type EnumType;
		return static_cast<key_state>(static_cast<EnumType>(a) | b);
	}

	inline key_state operator& (key_state a, std::underlying_type<key_state>::type b)
	{
		typedef std::underlying_type<key_state>::type EnumType;
		return static_cast<key_state>(static_cast<EnumType>(a) & b);
	}

	inline std::underlying_type<key_state>::type operator~ (key_state a)
	{
		return ~static_cast<std::underlying_type<key_state>::type>(a);
	}

	inline key_state& operator |= (key_state& a, key_state b)
	{
		return a = a | b;
	}

	inline key_state& operator &= (key_state& a, key_state b)
	{
		return a = a & b;
	}

	inline key_state& operator |= (key_state& a, std::underlying_type<key_state>::type b)
	{
		return a = a | static_cast<key_state>(b);
	}

	inline key_state& operator &= (key_state& a, std::underlying_type<key_state>::type b)
	{
		return a = a & static_cast<key_state>(b);
	}
}
