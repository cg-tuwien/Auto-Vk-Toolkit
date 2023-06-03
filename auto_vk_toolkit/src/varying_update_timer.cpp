#include "varying_update_timer.hpp"

namespace avk
{
	extern double get_context_time();

	varying_update_timer::varying_update_timer()
		: mStartTime(0.0),
		mLastTime(0.0),
		mAbsTime(0.0),
		mTimeSinceStart(0.0),
		mDeltaTime(0.0)
	{
		mAbsTime = mStartTime = get_context_time();
	}

	timer_frame_type varying_update_timer::tick()
	{
		mAbsTime = get_context_time();
		mTimeSinceStart = mAbsTime - mStartTime;
		mDeltaTime = mTimeSinceStart - mLastTime;
		mLastTime = mTimeSinceStart;
		return timer_frame_type::any; // ALWAYS both, update and render
	}

	float varying_update_timer::absolute_time() const
	{
		return static_cast<float>(mAbsTime);
	}

	float varying_update_timer::time_since_start() const
	{
		return static_cast<float>(mTimeSinceStart);
	}

	float varying_update_timer::fixed_delta_time() const
	{
		return static_cast<float>(mDeltaTime);
	}

	float varying_update_timer::delta_time() const
	{
		return static_cast<float>(mDeltaTime);
	}

	float varying_update_timer::time_scale() const
	{
		return 1.0f;
	}

	double varying_update_timer::absolute_time_dp() const
	{
		return mAbsTime;
	}

	double varying_update_timer::time_since_start_dp() const
	{
		return mTimeSinceStart;
	}

	double varying_update_timer::fixed_delta_time_dp() const
	{
		return mDeltaTime;
	}

	double varying_update_timer::delta_time_dp() const
	{
		return mDeltaTime;
	}

	double varying_update_timer::time_scale_dp() const
	{
		return 1.0;
	}

	// Defalt to a varying_update_timer if no other has been set:
	void set_default_timer()
	{
		static varying_update_timer sDefaultTimer;
		timer_reference() = &sDefaultTimer;
	}
}
