#pragma once

namespace cgb
{
	/** 
	 *	@type	F		Required format: void(command_buffer_t& _CommandBuffer, int64_t _InFlightIndex)
	 */
	template <typename F>
	std::vector<command_buffer> record_command_buffers_for_all_in_flight_frames(window* aWindow, F aRecordingFunction)
	{
		if (nullptr == aWindow) {
			aWindow = cgb::context().main_window();
		}
		const auto numInFlight = aWindow->number_of_in_flight_frames();
		auto commandBuffers = cgb::context().graphics_queue().create_command_buffers(static_cast<uint32_t>(numInFlight));

		for (int64_t inFlightIndex = 0; inFlightIndex < numInFlight; ++inFlightIndex) {
			command_buffer& cmdBfr = commandBuffers[inFlightIndex];
			cmdBfr->begin_recording();
			aRecordingFunction(static_cast<command_buffer_t&>(cmdBfr), inFlightIndex);
			cmdBfr->end_recording();
		}

		return commandBuffers;
	}
}
