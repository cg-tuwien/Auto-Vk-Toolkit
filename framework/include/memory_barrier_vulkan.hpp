#pragma once

namespace cgb
{
	enum struct memory_barrier_type { global, buffer, image };

	/**	Defines a memory barrier. 
	 */
	class memory_barrier
	{
	private:
		vk::AccessFlags mSrcAccessMask;
		vk::AccessFlags mDstAccessMask;
	};
}
