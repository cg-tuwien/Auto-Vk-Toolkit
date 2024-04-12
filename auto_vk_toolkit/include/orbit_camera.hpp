#pragma once

#include "camera.hpp"
#include "sequential_invoker.hpp"

namespace avk
{
	// An orbit camera or arcball camera, mostly by clicking and dragging the mouse.
	// Controls
	//  - Left click down ............... Camera rotates around rotation center if the mouse is moved
	//  - Right click down .............. Camera and rotation center is moved with the mouse movements.
	//  - Scroll ........................ Move towards or away from rotation center.
	//  - Alt || right click + scroll ... Move camera and rotation center in forward/back direction.
	//  - Ctrl .......................... Slower movements
	//  - Shift ......................... Faster movements
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

        /** Gets the current pivot distance
         *  @return The currently used pivot distance.
         */
        float pivot_distance() const;
		// Gets the current rotation speed
		auto rotation_speed() const { return mRotationSpeed; }
		// Gets the current pivot distance speed
		auto pivot_distance_speed() const { return mPivotDistanceSpeed; }
		// Gets the current zoom speed
		auto zoom_speed() const { return mZoomSpeed; }
		// Gets the current lateral speed
		auto lateral_speed() const { return mLateralSpeed; }
		// Gets the current fast multiplier, i.e., factor when fast movement is enabled
		auto fast_multiplier() const { return mFastMultiplier; }
		// Gets the current slow multiplier, i.e., factor when slow movement is enabled
		auto slow_multiplier() const { return mSlowMultiplier; }

		/**	Sets the pivot distance, which is the distance along the front vector
		 *	of the camera which this camera orbits around.
		 *	Note: The passed value might get clamped to min/max bounds.
		 *	@param aDistanceFromCamera	The desired distance along the front vector.
		 */
		void set_pivot_distance(float aDistanceFromCamera);
		// Sets the current rotation speed
		void rotation_speed(float value) { mRotationSpeed = value; }
		// Sets the current pivot distance speed
		void pivot_distance_speed(float value) { mPivotDistanceSpeed = value; }
		// Sets the current zoom speed
		void zoom_speed(float value) { mZoomSpeed = value; }
		// Sets the current lateral speed
		void lateral_speed(glm::vec2 value) { mLateralSpeed = value; }
		// Sets the current fast multiplier, i.e., factor when fast movement is enabled
		void fast_multiplier(float value) { mFastMultiplier = value; }
		// Sets the current slow multiplier, i.e., factor when slow movement is enabled
		void slow_multiplier(float value) { mSlowMultiplier = value; }

	private:
		void calculate_lateral_speed();

	protected:
		float mRotationSpeed;
		float mPivotDistance;
		float mPivotDistanceSpeed;
		float mZoomSpeed;
		glm::vec2 mLateralSpeed;
		float mFastMultiplier;
		float mSlowMultiplier;
	};
}
