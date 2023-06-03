#include "fixed_update_timer.hpp"

namespace avk
{
	extern double get_context_time();

	fixed_update_timer::fixed_update_timer() :
		mTimeSinceStart(0.0),
		mLastTime(0.0),
		mDeltaTime(0.0),
		mMinRenderHz(1.0),
		mMaxRenderDeltaTime(1.0 / 1.0),
		mCurrentRenderHz(0.0),
		mFixedHz(60.0),
		mFixedDeltaTime(1.0 / 60.0)
	{
		mLastFixedTick = mAbsTime = mStartTime = get_context_time();
		mNextFixedTick = mLastFixedTick + mFixedDeltaTime;
	}

	timer_frame_type fixed_update_timer::tick()
	{
		mAbsTime = get_context_time();
		mTimeSinceStart = mAbsTime - mStartTime;

		auto dt = mTimeSinceStart - mLastTime;

		// should we simulate or render?
		if (mAbsTime > mNextFixedTick && dt < mMaxRenderDeltaTime)
		{
			mLastFixedTick = mNextFixedTick;
			mNextFixedTick += mFixedDeltaTime;
			return timer_frame_type::update; // update/simulation only
		}

		mDeltaTime = dt;
		mLastTime = mTimeSinceStart;
		mCurrentRenderHz = 1.0 / mDeltaTime;
		return timer_frame_type::render; // render only
	}

	void fixed_update_timer::set_max_render_delta_time(double pMaxRenderDt)
	{
		mMaxRenderDeltaTime = pMaxRenderDt;
		mMinRenderHz = 1.0 / mMaxRenderDeltaTime;
	}

	void fixed_update_timer::set_fixed_simulation_hertz(double pFixedSimulationHz)
	{
		mFixedHz = pFixedSimulationHz;
		mFixedDeltaTime = 1.0 / mFixedHz;
	}

	float fixed_update_timer::absolute_time() const
	{
		return static_cast<float>(mAbsTime);
	}

	float fixed_update_timer::time_since_start() const
	{
		return static_cast<float>(mTimeSinceStart);
	}

	float fixed_update_timer::fixed_delta_time() const
	{
		return static_cast<float>(mFixedDeltaTime);
	}

	float fixed_update_timer::delta_time() const
	{
		return static_cast<float>(mDeltaTime);
	}

	float fixed_update_timer::time_scale() const
	{
		return 1.0f;
	}

	double fixed_update_timer::absolute_time_dp() const
	{
		return mAbsTime;
	}

	double fixed_update_timer::time_since_start_dp() const
	{
		return mTimeSinceStart;
	}

	double fixed_update_timer::fixed_delta_time_dp() const
	{
		return mFixedDeltaTime;
	}

	double fixed_update_timer::delta_time_dp() const
	{
		return mDeltaTime;
	}

	double fixed_update_timer::time_scale_dp() const
	{
		return 1.0;
	}
}
