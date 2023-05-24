#pragma once
#include "timer_interface.hpp"

namespace avk
{
	/**	@brief Timer enabling a fixed update rate
	 *
	 *	This timer_interface has a fixed simulation rate (i.e. constant fixed_delta_time)
	 *  and variable rendering rate (i.e. as many render fps as possible, with
	 *  variable rendering delta time). It also provides a minimum 
	 *  render-fps functionality (use @ref set_min_render_hz)
	 */
	class fixed_update_timer : public timer_interface
	{
	public:
		fixed_update_timer();

		timer_frame_type tick();

		void set_max_render_delta_time(double pMaxRenderDt);
		void set_fixed_simulation_hertz(double pFixedSimulationHz);

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
		double mCurrentRenderHz;
		double mMinRenderHz;
		double mMaxRenderDeltaTime;

		double mFixedDeltaTime;
		double mFixedHz;
		double mLastFixedTick;
		double mNextFixedTick;
	};
}
