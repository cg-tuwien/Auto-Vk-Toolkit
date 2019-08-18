#include "std_helpers.h"

namespace std
{
	/** Hash function for std::tuple<uint32_t, uint32_t>.
	 *	Used for, e.g., vulkan::mDistinctQueues which stores a tuple of queue family index and queue index.
	 */
	template<> struct hash<std::tuple<uint32_t, uint32_t>>
	{
		size_t operator()(std::tuple<uint32_t, uint32_t> const& v) const noexcept
		{
			auto const h0(hash<uint32_t>{}(std::get<0>(v)));
			auto const h1(hash<uint32_t>{}(std::get<1>(v)));
			return ((h0 ^ (h1 << 1)) >> 1);
			// I have no idea what I'm doing.
			// But hopefully, he has: https://prateekvjoshi.com/2014/06/05/using-hash-function-in-c-for-user-defined-classes/
		}
	};
}
