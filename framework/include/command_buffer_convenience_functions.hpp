#pragma once

namespace cgb
{
	/** A convenience funtion which invokes a callback function (in-flight-index)-number of times,
	 *	passing the appropriate in-flight-index to the callback.
	 *	
	 *	@type	F		void(int64_t inFlightIndex)
	 */
	template <typename F>
	void invoke_for_all_in_flight_frames(window* aWindow, F aCallback)
	{
		if (nullptr == aWindow) {
			aWindow = cgb::context().main_window();
		}
		const auto numInFlight = aWindow->number_of_in_flight_frames();
		for (int64_t inFlightIndex = 0; inFlightIndex < numInFlight; ++inFlightIndex) {
			aCallback(inFlightIndex);
		}
	}
	
	/** A convenience funtion which creates as many command buffers as there are in-flight-frames and
	 *	invokes a callback function (can be used to to record commands into the command buffer) for
	 *	each of them, also passing the appropriate in-flight-index to the callback.
	 *	
	 *	@type	F		Function signature: void(command_buffer_t& commandBuffer, int64_t inFlightIndex)
	 */
	template <typename F>
	std::vector<command_buffer> record_command_buffers_for_all_in_flight_frames(window* aWindow, cgb::device_queue& aQueue, F aCallback)
	{
		if (nullptr == aWindow) {
			aWindow = cgb::context().main_window();
		}
		const auto numInFlight = aWindow->number_of_in_flight_frames();
		auto commandBuffers = cgb::command_pool::create_command_buffers(aQueue, static_cast<uint32_t>(numInFlight));

		for (int64_t inFlightIndex = 0; inFlightIndex < numInFlight; ++inFlightIndex) {
			command_buffer& cmdBfr = commandBuffers[inFlightIndex];
			cmdBfr->begin_recording();
			aCallback(static_cast<command_buffer_t&>(cmdBfr), inFlightIndex);
			cmdBfr->end_recording();
		}

		return commandBuffers;
	}
}
