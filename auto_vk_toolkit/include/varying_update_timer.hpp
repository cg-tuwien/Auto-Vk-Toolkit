#pragma once

#include "timer_interface.hpp"

namespace avk
{
	/**	@brief Basic timer_interface which only supports time steps with varying delta time
	 *
	 *	This kind of timer_interface leads to as many updates/renders as possible.
	 *	Beware that there is no fixed time step and, thus, no @ref fixed_delta_time
	 *	Time between frames is always varying.
	 */
	class varying_update_timer : public timer_interface
	{
	public:
		varying_update_timer();

		timer_frame_type tick();

		float absolute_time() const override;
		float time_since_start() const override;
		float fixed_delta_time() const override;
		float delta_time() const override;
		float time_scale() const override;
		double absolute_time_dp() const override;
		double time_since_start_dp() const override;
		double fixed_delta_time_dp() const override;
		double delta_time_dp() const override;
		double time_scale_dp() const override;

	private:
		double mStartTime;
		double mAbsTime;
		double mTimeSinceStart;
		double mLastTime;
		double mDeltaTime;
	};
}
