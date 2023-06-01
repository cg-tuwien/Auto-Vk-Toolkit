#pragma once

#include "camera.hpp"
#include "sequential_invoker.hpp"

namespace avk
{
	// An orbit camera or arcball camera, mostly by clicking and dragging the mouse.
	// Controls
	//  - Left click down .... Camera rotates around rotation center if the mouse is moved
	//  - Right click down ... Camera is moved with the mouse movements (rotation center moves along)
	//  - Scroll ............. Move towards or away from rotation center.
	//                         If too close/too far away, rotation center is moved too.
	class orbit_camera : public camera, public invokee
	{
	public:
		orbit_camera(std::string aName = "orbit_camera", bool aIsEnabled = true);
		orbit_camera(orbit_camera&&) noexcept = default;
		orbit_camera(const orbit_camera&) noexcept = default;
		orbit_camera& operator=(orbit_camera&&) noexcept = default;
		orbit_camera& operator=(const orbit_camera&) noexcept = default;
		~orbit_camera() override = default;

		// Invoked when this camera becomes active
		void on_enable() override;
		// Invoked when this camera becomes inactive
		void on_disable() override;
		// Invoked every frame to handle input and update the camera's position
		void update() override;

	private:
		void calculate_lateral_speed();

	protected:
		float mRotationSpeed;
		float mPivotDistance;
		float mPivotDistanceSpeed;
		float mMinPivotDistance;
		float mMaxPivotDistance;
		float mPivotDistanceSlowDownRange;
		float mLateralSpeed;
		float mFastMultiplier;
		float mSlowMultiplier;
	};
}
