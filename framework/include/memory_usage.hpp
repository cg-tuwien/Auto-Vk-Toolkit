#pragma once

namespace cgb
{
	/** Specifies for a buffer, where the buffer's memory will be located */
	enum struct memory_usage
	{
		/** Buffer's memory will be visible and can be mapped on the host. */
		host_visible,

		/** Buffer's memory will be visible and coherently mappable on the host. */
		host_coherent,

		/** Buffer's memory will be visible on the host, but in cached mode. I.e. reads might be slower  */
		host_cached,

		/** Buffer's memory is accessible on the GPU only, i.e. not visible on the host at all. */
		device,

		/** Buffer's memory is accessible on the GPU only, but its data might be transfered back to the host. */
		device_readback,

		/** Buffer's memory is accessible on the GPU only and allows protected queue operations to access the memory. */
		device_protected
	};
}
