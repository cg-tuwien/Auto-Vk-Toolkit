#pragma once

namespace cgb
{
	/** 
	 *	@type	F		Required format: void(command_buffer& _CommandBuffer, int64_t _InFlightIndex)
	 */
	template <typename F>
	std::vector<command_buffer> record_command_buffers_for_all_in_flight_frames(window* _Window, F _RecordFunction)
	{
		if (nullptr == _Window) {
			_Window = cgb::context().main_window();
		}
		const auto numInFlight = _Window->number_of_in_flight_frames();
		auto commandBuffers = cgb::context().graphics_queue().pool().get_command_buffers(
			static_cast<uint32_t>(numInFlight), 
			vk::CommandBufferUsageFlagBits::eSimultaneousUse); // TODO: What does eSimultaneousUse mean?

		for (int64_t in_flight_index = 0; in_flight_index < numInFlight; ++in_flight_index) {
			auto& cmdBfr = commandBuffers[in_flight_index];
			cmdBfr.begin_recording();
			_RecordFunction(cmdBfr, in_flight_index);
			cmdBfr.end_recording();
		}

		return commandBuffers;
	}
}
