#pragma once

namespace cgb
{
	class command_pool;

	/** TBD */
	class command_buffer
	{
	public:
		command_buffer() = default;
		command_buffer(command_buffer&&) noexcept = default;
		command_buffer(const command_buffer&) = delete;
		command_buffer& operator=(command_buffer&&) noexcept = default;
		command_buffer& operator=(const command_buffer&) = delete;
		~command_buffer() = default;

	};

}
